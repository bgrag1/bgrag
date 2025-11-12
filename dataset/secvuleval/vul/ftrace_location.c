unsigned long ftrace_location(unsigned long ip)
{
	struct dyn_ftrace *rec;
	unsigned long offset;
	unsigned long size;

	rec = lookup_rec(ip, ip);
	if (!rec) {
		if (!kallsyms_lookup_size_offset(ip, &size, &offset))
			goto out;

		/* map sym+0 to __fentry__ */
		if (!offset)
			rec = lookup_rec(ip, ip + size - 1);
	}

	if (rec)
		return rec->ip;

out:
	return 0;
}
