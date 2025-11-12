static void ip6_negative_advice(struct sock *sk,
				struct dst_entry *dst)
{
	struct rt6_info *rt = dst_rt6_info(dst);

	if (rt->rt6i_flags & RTF_CACHE) {
		rcu_read_lock();
		if (rt6_check_expired(rt)) {
			/* counteract the dst_release() in sk_dst_reset() */
			dst_hold(dst);
			sk_dst_reset(sk);

			rt6_remove_exception_rt(rt);
		}
		rcu_read_unlock();
		return;
	}
	sk_dst_reset(sk);
}
