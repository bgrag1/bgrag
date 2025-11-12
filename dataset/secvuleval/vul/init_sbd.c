static struct gfs2_sbd *init_sbd(struct super_block *sb)
{
	struct gfs2_sbd *sdp;
	struct address_space *mapping;

	sdp = kzalloc(sizeof(struct gfs2_sbd), GFP_KERNEL);
	if (!sdp)
		return NULL;

	sdp->sd_vfs = sb;
	sdp->sd_lkstats = alloc_percpu(struct gfs2_pcpu_lkstats);
	if (!sdp->sd_lkstats)
		goto fail;
	sb->s_fs_info = sdp;

	set_bit(SDF_NOJOURNALID, &sdp->sd_flags);
	gfs2_tune_init(&sdp->sd_tune);

	init_waitqueue_head(&sdp->sd_kill_wait);
	init_waitqueue_head(&sdp->sd_async_glock_wait);
	atomic_set(&sdp->sd_glock_disposal, 0);
	init_completion(&sdp->sd_locking_init);
	init_completion(&sdp->sd_wdack);
	spin_lock_init(&sdp->sd_statfs_spin);

	spin_lock_init(&sdp->sd_rindex_spin);
	sdp->sd_rindex_tree.rb_node = NULL;

	INIT_LIST_HEAD(&sdp->sd_jindex_list);
	spin_lock_init(&sdp->sd_jindex_spin);
	mutex_init(&sdp->sd_jindex_mutex);
	init_completion(&sdp->sd_journal_ready);

	INIT_LIST_HEAD(&sdp->sd_quota_list);
	mutex_init(&sdp->sd_quota_mutex);
	mutex_init(&sdp->sd_quota_sync_mutex);
	init_waitqueue_head(&sdp->sd_quota_wait);
	spin_lock_init(&sdp->sd_bitmap_lock);

	INIT_LIST_HEAD(&sdp->sd_sc_inodes_list);

	mapping = &sdp->sd_aspace;

	address_space_init_once(mapping);
	mapping->a_ops = &gfs2_rgrp_aops;
	mapping->host = sb->s_bdev->bd_inode;
	mapping->flags = 0;
	mapping_set_gfp_mask(mapping, GFP_NOFS);
	mapping->i_private_data = NULL;
	mapping->writeback_index = 0;

	spin_lock_init(&sdp->sd_log_lock);
	atomic_set(&sdp->sd_log_pinned, 0);
	INIT_LIST_HEAD(&sdp->sd_log_revokes);
	INIT_LIST_HEAD(&sdp->sd_log_ordered);
	spin_lock_init(&sdp->sd_ordered_lock);

	init_waitqueue_head(&sdp->sd_log_waitq);
	init_waitqueue_head(&sdp->sd_logd_waitq);
	spin_lock_init(&sdp->sd_ail_lock);
	INIT_LIST_HEAD(&sdp->sd_ail1_list);
	INIT_LIST_HEAD(&sdp->sd_ail2_list);

	init_rwsem(&sdp->sd_log_flush_lock);
	atomic_set(&sdp->sd_log_in_flight, 0);
	init_waitqueue_head(&sdp->sd_log_flush_wait);
	mutex_init(&sdp->sd_freeze_mutex);

	return sdp;

fail:
	free_sbd(sdp);
	return NULL;
}
