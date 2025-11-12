static int tls_decrypt_sg(struct sock *sk, struct iov_iter *out_iov,
			  struct scatterlist *out_sg,
			  struct tls_decrypt_arg *darg)
{
	struct tls_context *tls_ctx = tls_get_ctx(sk);
	struct tls_sw_context_rx *ctx = tls_sw_ctx_rx(tls_ctx);
	struct tls_prot_info *prot = &tls_ctx->prot_info;
	int n_sgin, n_sgout, aead_size, err, pages = 0;
	struct sk_buff *skb = tls_strp_msg(ctx);
	const struct strp_msg *rxm = strp_msg(skb);
	const struct tls_msg *tlm = tls_msg(skb);
	struct aead_request *aead_req;
	struct scatterlist *sgin = NULL;
	struct scatterlist *sgout = NULL;
	const int data_len = rxm->full_len - prot->overhead_size;
	int tail_pages = !!prot->tail_size;
	struct tls_decrypt_ctx *dctx;
	struct sk_buff *clear_skb;
	int iv_offset = 0;
	u8 *mem;

	n_sgin = skb_nsg(skb, rxm->offset + prot->prepend_size,
			 rxm->full_len - prot->prepend_size);
	if (n_sgin < 1)
		return n_sgin ?: -EBADMSG;

	if (darg->zc && (out_iov || out_sg)) {
		clear_skb = NULL;

		if (out_iov)
			n_sgout = 1 + tail_pages +
				iov_iter_npages_cap(out_iov, INT_MAX, data_len);
		else
			n_sgout = sg_nents(out_sg);
	} else {
		darg->zc = false;

		clear_skb = tls_alloc_clrtxt_skb(sk, skb, rxm->full_len);
		if (!clear_skb)
			return -ENOMEM;

		n_sgout = 1 + skb_shinfo(clear_skb)->nr_frags;
	}

	/* Increment to accommodate AAD */
	n_sgin = n_sgin + 1;

	/* Allocate a single block of memory which contains
	 *   aead_req || tls_decrypt_ctx.
	 * Both structs are variable length.
	 */
	aead_size = sizeof(*aead_req) + crypto_aead_reqsize(ctx->aead_recv);
	aead_size = ALIGN(aead_size, __alignof__(*dctx));
	mem = kmalloc(aead_size + struct_size(dctx, sg, size_add(n_sgin, n_sgout)),
		      sk->sk_allocation);
	if (!mem) {
		err = -ENOMEM;
		goto exit_free_skb;
	}

	/* Segment the allocated memory */
	aead_req = (struct aead_request *)mem;
	dctx = (struct tls_decrypt_ctx *)(mem + aead_size);
	dctx->sk = sk;
	sgin = &dctx->sg[0];
	sgout = &dctx->sg[n_sgin];

	/* For CCM based ciphers, first byte of nonce+iv is a constant */
	switch (prot->cipher_type) {
	case TLS_CIPHER_AES_CCM_128:
		dctx->iv[0] = TLS_AES_CCM_IV_B0_BYTE;
		iv_offset = 1;
		break;
	case TLS_CIPHER_SM4_CCM:
		dctx->iv[0] = TLS_SM4_CCM_IV_B0_BYTE;
		iv_offset = 1;
		break;
	}

	/* Prepare IV */
	if (prot->version == TLS_1_3_VERSION ||
	    prot->cipher_type == TLS_CIPHER_CHACHA20_POLY1305) {
		memcpy(&dctx->iv[iv_offset], tls_ctx->rx.iv,
		       prot->iv_size + prot->salt_size);
	} else {
		err = skb_copy_bits(skb, rxm->offset + TLS_HEADER_SIZE,
				    &dctx->iv[iv_offset] + prot->salt_size,
				    prot->iv_size);
		if (err < 0)
			goto exit_free;
		memcpy(&dctx->iv[iv_offset], tls_ctx->rx.iv, prot->salt_size);
	}
	tls_xor_iv_with_seq(prot, &dctx->iv[iv_offset], tls_ctx->rx.rec_seq);

	/* Prepare AAD */
	tls_make_aad(dctx->aad, rxm->full_len - prot->overhead_size +
		     prot->tail_size,
		     tls_ctx->rx.rec_seq, tlm->control, prot);

	/* Prepare sgin */
	sg_init_table(sgin, n_sgin);
	sg_set_buf(&sgin[0], dctx->aad, prot->aad_size);
	err = skb_to_sgvec(skb, &sgin[1],
			   rxm->offset + prot->prepend_size,
			   rxm->full_len - prot->prepend_size);
	if (err < 0)
		goto exit_free;

	if (clear_skb) {
		sg_init_table(sgout, n_sgout);
		sg_set_buf(&sgout[0], dctx->aad, prot->aad_size);

		err = skb_to_sgvec(clear_skb, &sgout[1], prot->prepend_size,
				   data_len + prot->tail_size);
		if (err < 0)
			goto exit_free;
	} else if (out_iov) {
		sg_init_table(sgout, n_sgout);
		sg_set_buf(&sgout[0], dctx->aad, prot->aad_size);

		err = tls_setup_from_iter(out_iov, data_len, &pages, &sgout[1],
					  (n_sgout - 1 - tail_pages));
		if (err < 0)
			goto exit_free_pages;

		if (prot->tail_size) {
			sg_unmark_end(&sgout[pages]);
			sg_set_buf(&sgout[pages + 1], &dctx->tail,
				   prot->tail_size);
			sg_mark_end(&sgout[pages + 1]);
		}
	} else if (out_sg) {
		memcpy(sgout, out_sg, n_sgout * sizeof(*sgout));
	}
	dctx->free_sgout = !!pages;

	/* Prepare and submit AEAD request */
	err = tls_do_decryption(sk, sgin, sgout, dctx->iv,
				data_len + prot->tail_size, aead_req, darg);
	if (err)
		goto exit_free_pages;

	darg->skb = clear_skb ?: tls_strp_msg(ctx);
	clear_skb = NULL;

	if (unlikely(darg->async)) {
		err = tls_strp_msg_hold(&ctx->strp, &ctx->async_hold);
		if (err)
			__skb_queue_tail(&ctx->async_hold, darg->skb);
		return err;
	}

	if (prot->tail_size)
		darg->tail = dctx->tail;

exit_free_pages:
	/* Release the pages in case iov was mapped to pages */
	for (; pages > 0; pages--)
		put_page(sg_page(&sgout[pages]));
exit_free:
	kfree(mem);
exit_free_skb:
	consume_skb(clear_skb);
	return err;
}
