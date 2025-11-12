struct tls_context *tls_ctx_create(struct sock *sk)
{
	struct inet_connection_sock *icsk = inet_csk(sk);
	struct tls_context *ctx;

	ctx = kzalloc(sizeof(*ctx), GFP_ATOMIC);
	if (!ctx)
		return NULL;

	mutex_init(&ctx->tx_lock);
	ctx->sk_proto = READ_ONCE(sk->sk_prot);
	ctx->sk = sk;
	/* Release semantic of rcu_assign_pointer() ensures that
	 * ctx->sk_proto is visible before changing sk->sk_prot in
	 * update_sk_prot(), and prevents reading uninitialized value in
	 * tls_{getsockopt, setsockopt}. Note that we do not need a
	 * read barrier in tls_{getsockopt,setsockopt} as there is an
	 * address dependency between sk->sk_proto->{getsockopt,setsockopt}
	 * and ctx->sk_proto.
	 */
	rcu_assign_pointer(icsk->icsk_ulp_data, ctx);
	return ctx;
}
