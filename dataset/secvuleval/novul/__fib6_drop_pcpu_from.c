static void __fib6_drop_pcpu_from(struct fib6_nh *fib6_nh,
				  const struct fib6_info *match,
				  const struct fib6_table *table)
{
	int cpu;

	if (!fib6_nh->rt6i_pcpu)
		return;

	rcu_read_lock();
	/* release the reference to this fib entry from
	 * all of its cached pcpu routes
	 */
	for_each_possible_cpu(cpu) {
		struct rt6_info **ppcpu_rt;
		struct rt6_info *pcpu_rt;

		ppcpu_rt = per_cpu_ptr(fib6_nh->rt6i_pcpu, cpu);

		/* Paired with xchg() in rt6_get_pcpu_route() */
		pcpu_rt = READ_ONCE(*ppcpu_rt);

		/* only dropping the 'from' reference if the cached route
		 * is using 'match'. The cached pcpu_rt->from only changes
		 * from a fib6_info to NULL (ip6_dst_destroy); it can never
		 * change from one fib6_info reference to another
		 */
		if (pcpu_rt && rcu_access_pointer(pcpu_rt->from) == match) {
			struct fib6_info *from;

			from = xchg((__force struct fib6_info **)&pcpu_rt->from, NULL);
			fib6_info_release(from);
		}
	}
	rcu_read_unlock();
}
