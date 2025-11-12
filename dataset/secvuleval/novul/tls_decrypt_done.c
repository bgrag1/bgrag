static void tls_decrypt_done(void *data, int err)
{
	struct aead_request *aead_req = data;
	struct crypto_aead *aead = crypto_aead_reqtfm(aead_req);
	struct scatterlist *sgout = aead_req->dst;
	struct tls_sw_context_rx *ctx;
	struct tls_decrypt_ctx *dctx;
	struct tls_context *tls_ctx;
	struct scatterlist *sg;
	unsigned int pages;
	struct sock *sk;
	int aead_size;

	/* If requests get too backlogged crypto API returns -EBUSY and calls
	 * ->complete(-EINPROGRESS) immediately followed by ->complete(0)
	 * to make waiting for backlog to flush with crypto_wait_req() easier.
	 * First wait converts -EBUSY -> -EINPROGRESS, and the second one
	 * -EINPROGRESS -> 0.
	 * We have a single struct crypto_async_request per direction, this
	 * scheme doesn't help us, so just ignore the first ->complete().
	 */
	if (err == -EINPROGRESS)
		return;

	aead_size = sizeof(*aead_req) + crypto_aead_reqsize(aead);
	aead_size = ALIGN(aead_size, __alignof__(*dctx));
	dctx = (void *)((u8 *)aead_req + aead_size);

	sk = dctx->sk;
	tls_ctx = tls_get_ctx(sk);
	ctx = tls_sw_ctx_rx(tls_ctx);

	/* Propagate if there was an err */
	if (err) {
		if (err == -EBADMSG)
			TLS_INC_STATS(sock_net(sk), LINUX_MIB_TLSDECRYPTERROR);
		ctx->async_wait.err = err;
		tls_err_abort(sk, err);
	}

	/* Free the destination pages if skb was not decrypted inplace */
	if (dctx->free_sgout) {
		/* Skip the first S/G entry as it points to AAD */
		for_each_sg(sg_next(sgout), sg, UINT_MAX, pages) {
			if (!sg)
				break;
			put_page(sg_page(sg));
		}
	}

	kfree(aead_req);

	if (atomic_dec_and_test(&ctx->decrypt_pending))
		complete(&ctx->async_wait.completion);
}
