static void ipv4_negative_advice(struct sock *sk,
				 struct dst_entry *dst)
{
	struct rtable *rt = dst_rtable(dst);

	if ((dst->obsolete > 0) ||
	    (rt->rt_flags & RTCF_REDIRECTED) ||
	    rt->dst.expires)
		sk_dst_reset(sk);
}
