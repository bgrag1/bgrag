unsigned long ftrace_location_range(unsigned long start, unsigned long end)
{
	struct dyn_ftrace *rec;
	unsigned long ip = 0;

	rcu_read_lock();
	rec = lookup_rec(start, end);
	if (rec)
		ip = rec->ip;
	rcu_read_unlock();

	return ip;
}
