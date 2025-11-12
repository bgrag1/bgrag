static void ext4_mb_choose_next_group_best_avail(struct ext4_allocation_context *ac,
		enum criteria *new_cr, ext4_group_t *group)
{
	struct ext4_sb_info *sbi = EXT4_SB(ac->ac_sb);
	struct ext4_group_info *grp = NULL;
	int i, order, min_order;
	unsigned long num_stripe_clusters = 0;

	if (unlikely(ac->ac_flags & EXT4_MB_CR_BEST_AVAIL_LEN_OPTIMIZED)) {
		if (sbi->s_mb_stats)
			atomic_inc(&sbi->s_bal_best_avail_bad_suggestions);
	}

	/*
	 * mb_avg_fragment_size_order() returns order in a way that makes
	 * retrieving back the length using (1 << order) inaccurate. Hence, use
	 * fls() instead since we need to know the actual length while modifying
	 * goal length.
	 */
	order = fls(ac->ac_g_ex.fe_len) - 1;
	min_order = order - sbi->s_mb_best_avail_max_trim_order;
	if (min_order < 0)
		min_order = 0;

	if (sbi->s_stripe > 0) {
		/*
		 * We are assuming that stripe size is always a multiple of
		 * cluster ratio otherwise __ext4_fill_super exists early.
		 */
		num_stripe_clusters = EXT4_NUM_B2C(sbi, sbi->s_stripe);
		if (1 << min_order < num_stripe_clusters)
			/*
			 * We consider 1 order less because later we round
			 * up the goal len to num_stripe_clusters
			 */
			min_order = fls(num_stripe_clusters) - 1;
	}

	if (1 << min_order < ac->ac_o_ex.fe_len)
		min_order = fls(ac->ac_o_ex.fe_len);

	for (i = order; i >= min_order; i--) {
		int frag_order;
		/*
		 * Scale down goal len to make sure we find something
		 * in the free fragments list. Basically, reduce
		 * preallocations.
		 */
		ac->ac_g_ex.fe_len = 1 << i;

		if (num_stripe_clusters > 0) {
			/*
			 * Try to round up the adjusted goal length to
			 * stripe size (in cluster units) multiple for
			 * efficiency.
			 */
			ac->ac_g_ex.fe_len = roundup(ac->ac_g_ex.fe_len,
						     num_stripe_clusters);
		}

		frag_order = mb_avg_fragment_size_order(ac->ac_sb,
							ac->ac_g_ex.fe_len);

		grp = ext4_mb_find_good_group_avg_frag_lists(ac, frag_order);
		if (grp) {
			*group = grp->bb_group;
			ac->ac_flags |= EXT4_MB_CR_BEST_AVAIL_LEN_OPTIMIZED;
			return;
		}
	}

	/* Reset goal length to original goal length before falling into CR_GOAL_LEN_SLOW */
	ac->ac_g_ex.fe_len = ac->ac_orig_goal_len;
	*new_cr = CR_GOAL_LEN_SLOW;
}
