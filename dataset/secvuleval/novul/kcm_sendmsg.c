static int kcm_sendmsg(struct socket *sock, struct msghdr *msg, size_t len)
{
	struct sock *sk = sock->sk;
	struct kcm_sock *kcm = kcm_sk(sk);
	struct sk_buff *skb = NULL, *head = NULL;
	size_t copy, copied = 0;
	long timeo = sock_sndtimeo(sk, msg->msg_flags & MSG_DONTWAIT);
	int eor = (sock->type == SOCK_DGRAM) ?
		  !(msg->msg_flags & MSG_MORE) : !!(msg->msg_flags & MSG_EOR);
	int err = -EPIPE;

	mutex_lock(&kcm->tx_mutex);
	lock_sock(sk);

	/* Per tcp_sendmsg this should be in poll */
	sk_clear_bit(SOCKWQ_ASYNC_NOSPACE, sk);

	if (sk->sk_err)
		goto out_error;

	if (kcm->seq_skb) {
		/* Previously opened message */
		head = kcm->seq_skb;
		skb = kcm_tx_msg(head)->last_skb;
		goto start;
	}

	/* Call the sk_stream functions to manage the sndbuf mem. */
	if (!sk_stream_memory_free(sk)) {
		kcm_push(kcm);
		set_bit(SOCK_NOSPACE, &sk->sk_socket->flags);
		err = sk_stream_wait_memory(sk, &timeo);
		if (err)
			goto out_error;
	}

	if (msg_data_left(msg)) {
		/* New message, alloc head skb */
		head = alloc_skb(0, sk->sk_allocation);
		while (!head) {
			kcm_push(kcm);
			err = sk_stream_wait_memory(sk, &timeo);
			if (err)
				goto out_error;

			head = alloc_skb(0, sk->sk_allocation);
		}

		skb = head;

		/* Set ip_summed to CHECKSUM_UNNECESSARY to avoid calling
		 * csum_and_copy_from_iter from skb_do_copy_data_nocache.
		 */
		skb->ip_summed = CHECKSUM_UNNECESSARY;
	}

start:
	while (msg_data_left(msg)) {
		bool merge = true;
		int i = skb_shinfo(skb)->nr_frags;
		struct page_frag *pfrag = sk_page_frag(sk);

		if (!sk_page_frag_refill(sk, pfrag))
			goto wait_for_memory;

		if (!skb_can_coalesce(skb, i, pfrag->page,
				      pfrag->offset)) {
			if (i == MAX_SKB_FRAGS) {
				struct sk_buff *tskb;

				tskb = alloc_skb(0, sk->sk_allocation);
				if (!tskb)
					goto wait_for_memory;

				if (head == skb)
					skb_shinfo(head)->frag_list = tskb;
				else
					skb->next = tskb;

				skb = tskb;
				skb->ip_summed = CHECKSUM_UNNECESSARY;
				continue;
			}
			merge = false;
		}

		if (msg->msg_flags & MSG_SPLICE_PAGES) {
			copy = msg_data_left(msg);
			if (!sk_wmem_schedule(sk, copy))
				goto wait_for_memory;

			err = skb_splice_from_iter(skb, &msg->msg_iter, copy,
						   sk->sk_allocation);
			if (err < 0) {
				if (err == -EMSGSIZE)
					goto wait_for_memory;
				goto out_error;
			}

			copy = err;
			skb_shinfo(skb)->flags |= SKBFL_SHARED_FRAG;
			sk_wmem_queued_add(sk, copy);
			sk_mem_charge(sk, copy);

			if (head != skb)
				head->truesize += copy;
		} else {
			copy = min_t(int, msg_data_left(msg),
				     pfrag->size - pfrag->offset);
			if (!sk_wmem_schedule(sk, copy))
				goto wait_for_memory;

			err = skb_copy_to_page_nocache(sk, &msg->msg_iter, skb,
						       pfrag->page,
						       pfrag->offset,
						       copy);
			if (err)
				goto out_error;

			/* Update the skb. */
			if (merge) {
				skb_frag_size_add(
					&skb_shinfo(skb)->frags[i - 1], copy);
			} else {
				skb_fill_page_desc(skb, i, pfrag->page,
						   pfrag->offset, copy);
				get_page(pfrag->page);
			}

			pfrag->offset += copy;
		}

		copied += copy;
		if (head != skb) {
			head->len += copy;
			head->data_len += copy;
		}

		continue;

wait_for_memory:
		kcm_push(kcm);
		err = sk_stream_wait_memory(sk, &timeo);
		if (err)
			goto out_error;
	}

	if (eor) {
		bool not_busy = skb_queue_empty(&sk->sk_write_queue);

		if (head) {
			/* Message complete, queue it on send buffer */
			__skb_queue_tail(&sk->sk_write_queue, head);
			kcm->seq_skb = NULL;
			KCM_STATS_INCR(kcm->stats.tx_msgs);
		}

		if (msg->msg_flags & MSG_BATCH) {
			kcm->tx_wait_more = true;
		} else if (kcm->tx_wait_more || not_busy) {
			err = kcm_write_msgs(kcm);
			if (err < 0) {
				/* We got a hard error in write_msgs but have
				 * already queued this message. Report an error
				 * in the socket, but don't affect return value
				 * from sendmsg
				 */
				pr_warn("KCM: Hard failure on kcm_write_msgs\n");
				report_csk_error(&kcm->sk, -err);
			}
		}
	} else {
		/* Message not complete, save state */
partial_message:
		if (head) {
			kcm->seq_skb = head;
			kcm_tx_msg(head)->last_skb = skb;
		}
	}

	KCM_STATS_ADD(kcm->stats.tx_bytes, copied);

	release_sock(sk);
	mutex_unlock(&kcm->tx_mutex);
	return copied;

out_error:
	kcm_push(kcm);

	if (sock->type == SOCK_SEQPACKET) {
		/* Wrote some bytes before encountering an
		 * error, return partial success.
		 */
		if (copied)
			goto partial_message;
		if (head != kcm->seq_skb)
			kfree_skb(head);
	} else {
		kfree_skb(head);
		kcm->seq_skb = NULL;
	}

	err = sk_stream_error(sk, msg->msg_flags, err);

	/* make sure we wake any epoll edge trigger waiter */
	if (unlikely(skb_queue_len(&sk->sk_write_queue) == 0 && err == -EAGAIN))
		sk->sk_write_space(sk);

	release_sock(sk);
	mutex_unlock(&kcm->tx_mutex);
	return err;
}
