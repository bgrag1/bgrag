static void sk_psock_verdict_data_ready(struct sock *sk)
{
	struct socket *sock = sk->sk_socket;
	const struct proto_ops *ops;
	int copied;

	trace_sk_data_ready(sk);

	if (unlikely(!sock))
		return;
	ops = READ_ONCE(sock->ops);
	if (!ops || !ops->read_skb)
		return;
	copied = ops->read_skb(sk, sk_psock_verdict_recv);
	if (copied >= 0) {
		struct sk_psock *psock;

		rcu_read_lock();
		psock = sk_psock(sk);
		if (psock)
			sk_psock_data_ready(sk, psock);
		rcu_read_unlock();
	}
}
