static struct rfcomm_session *rfcomm_process_rx(struct rfcomm_session *s)
{
	struct socket *sock = s->sock;
	struct sock *sk = sock->sk;
	struct sk_buff *skb;

	BT_DBG("session %p state %ld qlen %d", s, s->state, skb_queue_len(&sk->sk_receive_queue));

	/* Get data directly from socket receive queue without copying it. */
	while ((skb = skb_dequeue(&sk->sk_receive_queue))) {
		skb_orphan(skb);
		if (!skb_linearize(skb) && sk->sk_state != BT_CLOSED) {
			s = rfcomm_recv_frame(s, skb);
			if (!s)
				break;
		} else {
			kfree_skb(skb);
		}
	}

	if (s && (sk->sk_state == BT_CLOSED))
		s = rfcomm_session_close(s, sk->sk_err);

	return s;
}
