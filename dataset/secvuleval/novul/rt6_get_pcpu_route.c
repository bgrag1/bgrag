static struct rt6_info *rt6_get_pcpu_route(const struct fib6_result *res)
{
	struct rt6_info *pcpu_rt;

	pcpu_rt = this_cpu_read(*res->nh->rt6i_pcpu);

	if (pcpu_rt && pcpu_rt->sernum && !rt6_is_valid(pcpu_rt)) {
		struct rt6_info *prev, **p;

		p = this_cpu_ptr(res->nh->rt6i_pcpu);
		/* Paired with READ_ONCE() in __fib6_drop_pcpu_from() */
		prev = xchg(p, NULL);
		if (prev) {
			dst_dev_put(&prev->dst);
			dst_release(&prev->dst);
		}

		pcpu_rt = NULL;
	}

	return pcpu_rt;
}
