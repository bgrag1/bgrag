static void xfrm_negative_advice(struct sock *sk, struct dst_entry *dst)
{
	if (dst->obsolete)
		sk_dst_reset(sk);
}
