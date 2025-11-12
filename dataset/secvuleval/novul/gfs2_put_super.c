static void gfs2_put_super(struct super_block *sb)
{
	struct gfs2_sbd *sdp = sb->s_fs_info;
	struct gfs2_jdesc *jd;

	/* No more recovery requests */
	set_bit(SDF_NORECOVERY, &sdp->sd_flags);
	smp_mb();

	/* Wait on outstanding recovery */
restart:
	spin_lock(&sdp->sd_jindex_spin);
	list_for_each_entry(jd, &sdp->sd_jindex_list, jd_list) {
		if (!test_bit(JDF_RECOVERY, &jd->jd_flags))
			continue;
		spin_unlock(&sdp->sd_jindex_spin);
		wait_on_bit(&jd->jd_flags, JDF_RECOVERY,
			    TASK_UNINTERRUPTIBLE);
		goto restart;
	}
	spin_unlock(&sdp->sd_jindex_spin);

	if (!sb_rdonly(sb))
		gfs2_make_fs_ro(sdp);
	else {
		if (gfs2_withdrawing_or_withdrawn(sdp))
			gfs2_destroy_threads(sdp);

		gfs2_quota_cleanup(sdp);
	}

	WARN_ON(gfs2_withdrawing(sdp));

	/*  At this point, we're through modifying the disk  */

	/*  Release stuff  */

	gfs2_freeze_unlock(&sdp->sd_freeze_gh);

	iput(sdp->sd_jindex);
	iput(sdp->sd_statfs_inode);
	iput(sdp->sd_rindex);
	iput(sdp->sd_quota_inode);

	gfs2_glock_put(sdp->sd_rename_gl);
	gfs2_glock_put(sdp->sd_freeze_gl);

	if (!sdp->sd_args.ar_spectator) {
		if (gfs2_holder_initialized(&sdp->sd_journal_gh))
			gfs2_glock_dq_uninit(&sdp->sd_journal_gh);
		if (gfs2_holder_initialized(&sdp->sd_jinode_gh))
			gfs2_glock_dq_uninit(&sdp->sd_jinode_gh);
		brelse(sdp->sd_sc_bh);
		gfs2_glock_dq_uninit(&sdp->sd_sc_gh);
		gfs2_glock_dq_uninit(&sdp->sd_qc_gh);
		free_local_statfs_inodes(sdp);
		iput(sdp->sd_qc_inode);
	}

	gfs2_glock_dq_uninit(&sdp->sd_live_gh);
	gfs2_clear_rgrpd(sdp);
	gfs2_jindex_free(sdp);
	/*  Take apart glock structures and buffer lists  */
	gfs2_gl_hash_clear(sdp);
	truncate_inode_pages_final(&sdp->sd_aspace);
	gfs2_delete_debugfs_file(sdp);

	gfs2_sys_fs_del(sdp);
	free_sbd(sdp);
}
