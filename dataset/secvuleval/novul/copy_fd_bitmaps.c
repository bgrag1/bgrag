static inline void copy_fd_bitmaps(struct fdtable *nfdt, struct fdtable *ofdt,
			    unsigned int copy_words)
{
	unsigned int nwords = fdt_words(nfdt);

	bitmap_copy_and_extend(nfdt->open_fds, ofdt->open_fds,
			copy_words * BITS_PER_LONG, nwords * BITS_PER_LONG);
	bitmap_copy_and_extend(nfdt->close_on_exec, ofdt->close_on_exec,
			copy_words * BITS_PER_LONG, nwords * BITS_PER_LONG);
	bitmap_copy_and_extend(nfdt->full_fds_bits, ofdt->full_fds_bits,
			copy_words, nwords);
}
