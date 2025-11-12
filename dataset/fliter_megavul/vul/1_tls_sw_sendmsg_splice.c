static int tls_sw_sendmsg_splice(struct sock *sk, struct msghdr *msg,
				 struct sk_msg *msg_pl, size_t try_to_copy,
				 ssize_t *copied)
{
	struct page *page = NULL, **pages = &page;

	do {
		ssize_t part;
		size_t off;

		part = iov_iter_extract_pages(&msg->msg_iter, &pages,
					      try_to_copy, 1, 0, &off);
		if (part <= 0)
			return part ?: -EIO;

		if (WARN_ON_ONCE(!sendpage_ok(page))) {
			iov_iter_revert(&msg->msg_iter, part);
			return -EIO;
		}

		sk_msg_page_add(msg_pl, page, part, off);
		sk_mem_charge(sk, part);
		*copied += part;
		try_to_copy -= part;
	} while (try_to_copy && !sk_msg_full(msg_pl));

	return 0;
}