static int do_read_inode(struct inode *inode)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	struct f2fs_inode_info *fi = F2FS_I(inode);
	struct page *node_page;
	struct f2fs_inode *ri;
	projid_t i_projid;

	/* Check if ino is within scope */
	if (f2fs_check_nid_range(sbi, inode->i_ino))
		return -EINVAL;

	node_page = f2fs_get_node_page(sbi, inode->i_ino);
	if (IS_ERR(node_page))
		return PTR_ERR(node_page);

	ri = F2FS_INODE(node_page);

	inode->i_mode = le16_to_cpu(ri->i_mode);
	i_uid_write(inode, le32_to_cpu(ri->i_uid));
	i_gid_write(inode, le32_to_cpu(ri->i_gid));
	set_nlink(inode, le32_to_cpu(ri->i_links));
	inode->i_size = le64_to_cpu(ri->i_size);
	inode->i_blocks = SECTOR_FROM_BLOCK(le64_to_cpu(ri->i_blocks) - 1);

	inode_set_atime(inode, le64_to_cpu(ri->i_atime),
			le32_to_cpu(ri->i_atime_nsec));
	inode_set_ctime(inode, le64_to_cpu(ri->i_ctime),
			le32_to_cpu(ri->i_ctime_nsec));
	inode_set_mtime(inode, le64_to_cpu(ri->i_mtime),
			le32_to_cpu(ri->i_mtime_nsec));
	inode->i_generation = le32_to_cpu(ri->i_generation);
	if (S_ISDIR(inode->i_mode))
		fi->i_current_depth = le32_to_cpu(ri->i_current_depth);
	else if (S_ISREG(inode->i_mode))
		fi->i_gc_failures = le16_to_cpu(ri->i_gc_failures);
	fi->i_xattr_nid = le32_to_cpu(ri->i_xattr_nid);
	fi->i_flags = le32_to_cpu(ri->i_flags);
	if (S_ISREG(inode->i_mode))
		fi->i_flags &= ~F2FS_PROJINHERIT_FL;
	bitmap_zero(fi->flags, FI_MAX);
	fi->i_advise = ri->i_advise;
	fi->i_pino = le32_to_cpu(ri->i_pino);
	fi->i_dir_level = ri->i_dir_level;

	get_inline_info(inode, ri);

	fi->i_extra_isize = f2fs_has_extra_attr(inode) ?
					le16_to_cpu(ri->i_extra_isize) : 0;

	if (f2fs_sb_has_flexible_inline_xattr(sbi)) {
		fi->i_inline_xattr_size = le16_to_cpu(ri->i_inline_xattr_size);
	} else if (f2fs_has_inline_xattr(inode) ||
				f2fs_has_inline_dentry(inode)) {
		fi->i_inline_xattr_size = DEFAULT_INLINE_XATTR_ADDRS;
	} else {

		/*
		 * Previous inline data or directory always reserved 200 bytes
		 * in inode layout, even if inline_xattr is disabled. In order
		 * to keep inline_dentry's structure for backward compatibility,
		 * we get the space back only from inline_data.
		 */
		fi->i_inline_xattr_size = 0;
	}

	if (!sanity_check_inode(inode, node_page)) {
		f2fs_put_page(node_page, 1);
		set_sbi_flag(sbi, SBI_NEED_FSCK);
		f2fs_handle_error(sbi, ERROR_CORRUPTED_INODE);
		return -EFSCORRUPTED;
	}

	/* check data exist */
	if (f2fs_has_inline_data(inode) && !f2fs_exist_data(inode))
		__recover_inline_status(inode, node_page);

	/* try to recover cold bit for non-dir inode */
	if (!S_ISDIR(inode->i_mode) && !is_cold_node(node_page)) {
		f2fs_wait_on_page_writeback(node_page, NODE, true, true);
		set_cold_node(node_page, false);
		set_page_dirty(node_page);
	}

	/* get rdev by using inline_info */
	__get_inode_rdev(inode, node_page);

	if (!f2fs_need_inode_block_update(sbi, inode->i_ino))
		fi->last_disk_size = inode->i_size;

	if (fi->i_flags & F2FS_PROJINHERIT_FL)
		set_inode_flag(inode, FI_PROJ_INHERIT);

	if (f2fs_has_extra_attr(inode) && f2fs_sb_has_project_quota(sbi) &&
			F2FS_FITS_IN_INODE(ri, fi->i_extra_isize, i_projid))
		i_projid = (projid_t)le32_to_cpu(ri->i_projid);
	else
		i_projid = F2FS_DEF_PROJID;
	fi->i_projid = make_kprojid(&init_user_ns, i_projid);

	if (f2fs_has_extra_attr(inode) && f2fs_sb_has_inode_crtime(sbi) &&
			F2FS_FITS_IN_INODE(ri, fi->i_extra_isize, i_crtime)) {
		fi->i_crtime.tv_sec = le64_to_cpu(ri->i_crtime);
		fi->i_crtime.tv_nsec = le32_to_cpu(ri->i_crtime_nsec);
	}

	if (f2fs_has_extra_attr(inode) && f2fs_sb_has_compression(sbi) &&
					(fi->i_flags & F2FS_COMPR_FL)) {
		if (F2FS_FITS_IN_INODE(ri, fi->i_extra_isize,
					i_compress_flag)) {
			unsigned short compress_flag;

			atomic_set(&fi->i_compr_blocks,
					le64_to_cpu(ri->i_compr_blocks));
			fi->i_compress_algorithm = ri->i_compress_algorithm;
			fi->i_log_cluster_size = ri->i_log_cluster_size;
			compress_flag = le16_to_cpu(ri->i_compress_flag);
			fi->i_compress_level = compress_flag >>
						COMPRESS_LEVEL_OFFSET;
			fi->i_compress_flag = compress_flag &
					GENMASK(COMPRESS_LEVEL_OFFSET - 1, 0);
			fi->i_cluster_size = BIT(fi->i_log_cluster_size);
			set_inode_flag(inode, FI_COMPRESSED_FILE);
		}
	}

	init_idisk_time(inode);

	if (!sanity_check_extent_cache(inode, node_page)) {
		f2fs_put_page(node_page, 1);
		f2fs_handle_error(sbi, ERROR_CORRUPTED_INODE);
		return -EFSCORRUPTED;
	}

	/* Need all the flag bits */
	f2fs_init_read_extent_tree(inode, node_page);
	f2fs_init_age_extent_tree(inode);

	f2fs_put_page(node_page, 1);

	stat_inc_inline_xattr(inode);
	stat_inc_inline_inode(inode);
	stat_inc_inline_dir(inode);
	stat_inc_compr_inode(inode);
	stat_add_compr_blocks(inode, atomic_read(&fi->i_compr_blocks));

	return 0;
}
