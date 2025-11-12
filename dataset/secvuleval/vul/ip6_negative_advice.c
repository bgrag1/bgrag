static struct dst_entry *ip6_negative_advice(struct dst_entry *dst)
{
	struct rt6_info *rt = dst_rt6_info(dst);

	if (rt) {
		if (rt->rt6i_flags & RTF_CACHE) {
			rcu_read_lock();
			if (rt6_check_expired(rt)) {
				rt6_remove_exception_rt(rt);
				dst = NULL;
			}
			rcu_read_unlock();
		} else {
			dst_release(dst);
			dst = NULL;
		}
	}
	return dst;
}
