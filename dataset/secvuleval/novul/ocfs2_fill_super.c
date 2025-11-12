static int ocfs2_fill_super(struct super_block *sb, void *data, int silent)
{
	struct dentry *root;
	int status, sector_size;
	struct mount_options parsed_options;
	struct inode *inode = NULL;
	struct ocfs2_super *osb = NULL;
	struct buffer_head *bh = NULL;
	char nodestr[12];
	struct ocfs2_blockcheck_stats stats;

	trace_ocfs2_fill_super(sb, data, silent);

	if (!ocfs2_parse_options(sb, data, &parsed_options, 0)) {
		status = -EINVAL;
		goto out;
	}

	/* probe for superblock */
	status = ocfs2_sb_probe(sb, &bh, &sector_size, &stats);
	if (status < 0) {
		mlog(ML_ERROR, "superblock probe failed!\n");
		goto out;
	}

	status = ocfs2_initialize_super(sb, bh, sector_size, &stats);
	brelse(bh);
	bh = NULL;
	if (status < 0)
		goto out;

	osb = OCFS2_SB(sb);

	if (!ocfs2_check_set_options(sb, &parsed_options)) {
		status = -EINVAL;
		goto out_super;
	}
	osb->s_mount_opt = parsed_options.mount_opt;
	osb->s_atime_quantum = parsed_options.atime_quantum;
	osb->preferred_slot = parsed_options.slot;
	osb->osb_commit_interval = parsed_options.commit_interval;

	ocfs2_la_set_sizes(osb, parsed_options.localalloc_opt);
	osb->osb_resv_level = parsed_options.resv_level;
	osb->osb_dir_resv_level = parsed_options.resv_level;
	if (parsed_options.dir_resv_level == -1)
		osb->osb_dir_resv_level = parsed_options.resv_level;
	else
		osb->osb_dir_resv_level = parsed_options.dir_resv_level;

	status = ocfs2_verify_userspace_stack(osb, &parsed_options);
	if (status)
		goto out_super;

	sb->s_magic = OCFS2_SUPER_MAGIC;

	sb->s_flags = (sb->s_flags & ~(SB_POSIXACL | SB_NOSEC)) |
		((osb->s_mount_opt & OCFS2_MOUNT_POSIX_ACL) ? SB_POSIXACL : 0);

	/* Hard readonly mode only if: bdev_read_only, SB_RDONLY,
	 * heartbeat=none */
	if (bdev_read_only(sb->s_bdev)) {
		if (!sb_rdonly(sb)) {
			status = -EACCES;
			mlog(ML_ERROR, "Readonly device detected but readonly "
			     "mount was not specified.\n");
			goto out_super;
		}

		/* You should not be able to start a local heartbeat
		 * on a readonly device. */
		if (osb->s_mount_opt & OCFS2_MOUNT_HB_LOCAL) {
			status = -EROFS;
			mlog(ML_ERROR, "Local heartbeat specified on readonly "
			     "device.\n");
			goto out_super;
		}

		status = ocfs2_check_journals_nolocks(osb);
		if (status < 0) {
			if (status == -EROFS)
				mlog(ML_ERROR, "Recovery required on readonly "
				     "file system, but write access is "
				     "unavailable.\n");
			goto out_super;
		}

		ocfs2_set_ro_flag(osb, 1);

		printk(KERN_NOTICE "ocfs2: Readonly device (%s) detected. "
		       "Cluster services will not be used for this mount. "
		       "Recovery will be skipped.\n", osb->dev_str);
	}

	if (!ocfs2_is_hard_readonly(osb)) {
		if (sb_rdonly(sb))
			ocfs2_set_ro_flag(osb, 0);
	}

	status = ocfs2_verify_heartbeat(osb);
	if (status < 0)
		goto out_super;

	osb->osb_debug_root = debugfs_create_dir(osb->uuid_str,
						 ocfs2_debugfs_root);

	debugfs_create_file("fs_state", S_IFREG|S_IRUSR, osb->osb_debug_root,
			    osb, &ocfs2_osb_debug_fops);

	if (ocfs2_meta_ecc(osb)) {
		ocfs2_initialize_journal_triggers(sb, osb->s_journal_triggers);
		ocfs2_blockcheck_stats_debugfs_install( &osb->osb_ecc_stats,
							osb->osb_debug_root);
	}

	status = ocfs2_mount_volume(sb);
	if (status < 0)
		goto out_debugfs;

	if (osb->root_inode)
		inode = igrab(osb->root_inode);

	if (!inode) {
		status = -EIO;
		goto out_dismount;
	}

	osb->osb_dev_kset = kset_create_and_add(sb->s_id, NULL,
						&ocfs2_kset->kobj);
	if (!osb->osb_dev_kset) {
		status = -ENOMEM;
		mlog(ML_ERROR, "Unable to create device kset %s.\n", sb->s_id);
		goto out_dismount;
	}

	/* Create filecheck sysfs related directories/files at
	 * /sys/fs/ocfs2/<devname>/filecheck */
	if (ocfs2_filecheck_create_sysfs(osb)) {
		status = -ENOMEM;
		mlog(ML_ERROR, "Unable to create filecheck sysfs directory at "
			"/sys/fs/ocfs2/%s/filecheck.\n", sb->s_id);
		goto out_dismount;
	}

	root = d_make_root(inode);
	if (!root) {
		status = -ENOMEM;
		goto out_dismount;
	}

	sb->s_root = root;

	ocfs2_complete_mount_recovery(osb);

	if (ocfs2_mount_local(osb))
		snprintf(nodestr, sizeof(nodestr), "local");
	else
		snprintf(nodestr, sizeof(nodestr), "%u", osb->node_num);

	printk(KERN_INFO "ocfs2: Mounting device (%s) on (node %s, slot %d) "
	       "with %s data mode.\n",
	       osb->dev_str, nodestr, osb->slot_num,
	       osb->s_mount_opt & OCFS2_MOUNT_DATA_WRITEBACK ? "writeback" :
	       "ordered");

	atomic_set(&osb->vol_state, VOLUME_MOUNTED);
	wake_up(&osb->osb_mount_event);

	/* Now we can initialize quotas because we can afford to wait
	 * for cluster locks recovery now. That also means that truncation
	 * log recovery can happen but that waits for proper quota setup */
	if (!sb_rdonly(sb)) {
		status = ocfs2_enable_quotas(osb);
		if (status < 0) {
			/* We have to err-out specially here because
			 * s_root is already set */
			mlog_errno(status);
			atomic_set(&osb->vol_state, VOLUME_DISABLED);
			wake_up(&osb->osb_mount_event);
			return status;
		}
	}

	ocfs2_complete_quota_recovery(osb);

	/* Now we wake up again for processes waiting for quotas */
	atomic_set(&osb->vol_state, VOLUME_MOUNTED_QUOTAS);
	wake_up(&osb->osb_mount_event);

	/* Start this when the mount is almost sure of being successful */
	ocfs2_orphan_scan_start(osb);

	return status;

out_dismount:
	atomic_set(&osb->vol_state, VOLUME_DISABLED);
	wake_up(&osb->osb_mount_event);
	ocfs2_free_replay_slots(osb);
	ocfs2_dismount_volume(sb, 1);
	goto out;

out_debugfs:
	debugfs_remove_recursive(osb->osb_debug_root);
out_super:
	ocfs2_release_system_inodes(osb);
	kfree(osb->recovery_map);
	ocfs2_delete_osb(osb);
	kfree(osb);
out:
	mlog_errno(status);

	return status;
}
