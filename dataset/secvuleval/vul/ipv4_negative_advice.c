static struct dst_entry *ipv4_negative_advice(struct dst_entry *dst)
{
	struct rtable *rt = dst_rtable(dst);
	struct dst_entry *ret = dst;

	if (rt) {
		if (dst->obsolete > 0) {
			ip_rt_put(rt);
			ret = NULL;
		} else if ((rt->rt_flags & RTCF_REDIRECTED) ||
			   rt->dst.expires) {
			ip_rt_put(rt);
			ret = NULL;
		}
	}
	return ret;
}
