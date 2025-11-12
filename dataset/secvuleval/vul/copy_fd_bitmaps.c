static void copy_fd_bitmaps(struct fdtable *nfdt, struct fdtable *ofdt,
			    unsigned int count)
{
	unsigned int cpy, set;

	cpy = count / BITS_PER_BYTE;
	set = (nfdt->max_fds - count) / BITS_PER_BYTE;
	memcpy(nfdt->open_fds, ofdt->open_fds, cpy);
	memset((char *)nfdt->open_fds + cpy, 0, set);
	memcpy(nfdt->close_on_exec, ofdt->close_on_exec, cpy);
	memset((char *)nfdt->close_on_exec + cpy, 0, set);

	cpy = BITBIT_SIZE(count);
	set = BITBIT_SIZE(nfdt->max_fds) - cpy;
	memcpy(nfdt->full_fds_bits, ofdt->full_fds_bits, cpy);
	memset((char *)nfdt->full_fds_bits + cpy, 0, set);
}
