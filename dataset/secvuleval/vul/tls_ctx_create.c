struct tls_context *tls_ctx_create(struct sock *sk)
{
	struct inet_connection_sock *icsk = inet_csk(sk);
	struct tls_context *ctx;

	ctx = kzalloc(sizeof(*ctx), GFP_ATOMIC);
	if (!ctx)
		return NULL;

	mutex_init(&ctx->tx_lock);
	rcu_assign_pointer(icsk->icsk_ulp_data, ctx);
	ctx->sk_proto = READ_ONCE(sk->sk_prot);
	ctx->sk = sk;
	return ctx;
}
