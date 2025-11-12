unsigned long ftrace_location(unsigned long ip)
{
	unsigned long loc;
	unsigned long offset;
	unsigned long size;

	loc = ftrace_location_range(ip, ip);
	if (!loc) {
		if (!kallsyms_lookup_size_offset(ip, &size, &offset))
			goto out;

		/* map sym+0 to __fentry__ */
		if (!offset)
			loc = ftrace_location_range(ip, ip + size - 1);
	}

out:
	return loc;
}
