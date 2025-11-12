unsigned long ftrace_location_range(unsigned long start, unsigned long end)
{
	struct dyn_ftrace *rec;

	rec = lookup_rec(start, end);
	if (rec)
		return rec->ip;

	return 0;
}
