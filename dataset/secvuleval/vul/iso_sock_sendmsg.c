static int iso_sock_sendmsg(struct socket *sock, struct msghdr *msg,
			    size_t len)
{
	struct sock *sk = sock->sk;
	struct sk_buff *skb, **frag;
	size_t mtu;
	int err;

	BT_DBG("sock %p, sk %p", sock, sk);

	err = sock_error(sk);
	if (err)
		return err;

	if (msg->msg_flags & MSG_OOB)
		return -EOPNOTSUPP;

	lock_sock(sk);

	if (sk->sk_state != BT_CONNECTED) {
		release_sock(sk);
		return -ENOTCONN;
	}

	mtu = iso_pi(sk)->conn->hcon->hdev->iso_mtu;

	release_sock(sk);

	skb = bt_skb_sendmsg(sk, msg, len, mtu, HCI_ISO_DATA_HDR_SIZE, 0);
	if (IS_ERR(skb))
		return PTR_ERR(skb);

	len -= skb->len;

	BT_DBG("skb %p len %d", sk, skb->len);

	/* Continuation fragments */
	frag = &skb_shinfo(skb)->frag_list;
	while (len) {
		struct sk_buff *tmp;

		tmp = bt_skb_sendmsg(sk, msg, len, mtu, 0, 0);
		if (IS_ERR(tmp)) {
			kfree_skb(skb);
			return PTR_ERR(tmp);
		}

		*frag = tmp;

		len  -= tmp->len;

		skb->len += tmp->len;
		skb->data_len += tmp->len;

		BT_DBG("frag %p len %d", *frag, tmp->len);

		frag = &(*frag)->next;
	}

	lock_sock(sk);

	if (sk->sk_state == BT_CONNECTED)
		err = iso_send_frame(sk, skb);
	else
		err = -ENOTCONN;

	release_sock(sk);

	if (err < 0)
		kfree_skb(skb);
	return err;
}
