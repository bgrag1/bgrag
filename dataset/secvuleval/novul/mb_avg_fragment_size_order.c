static int mb_avg_fragment_size_order(struct super_block *sb, ext4_grpblk_t len)
{
	int order;

	/*
	 * We don't bother with a special lists groups with only 1 block free
	 * extents and for completely empty groups.
	 */
	order = fls(len) - 2;
	if (order < 0)
		return 0;
	if (order == MB_NUM_ORDERS(sb))
		order--;
	if (WARN_ON_ONCE(order > MB_NUM_ORDERS(sb)))
		order = MB_NUM_ORDERS(sb) - 1;
	return order;
}
