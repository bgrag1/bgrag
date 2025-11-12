struct inode *f2fs_iget(struct super_block *sb, unsigned long ino)
{
	struct f2fs_sb_info *sbi = F2FS_SB(sb);
	struct inode *inode;
	int ret = 0;

	inode = iget_locked(sb, ino);
	if (!inode)
		return ERR_PTR(-ENOMEM);

	if (!(inode->i_state & I_NEW)) {
		if (is_meta_ino(sbi, ino)) {
			f2fs_err(sbi, "inaccessible inode: %lu, run fsck to repair", ino);
			set_sbi_flag(sbi, SBI_NEED_FSCK);
			ret = -EFSCORRUPTED;
			trace_f2fs_iget_exit(inode, ret);
			iput(inode);
			f2fs_handle_error(sbi, ERROR_CORRUPTED_INODE);
			return ERR_PTR(ret);
		}

		trace_f2fs_iget(inode);
		return inode;
	}

	if (is_meta_ino(sbi, ino))
		goto make_now;

	ret = do_read_inode(inode);
	if (ret)
		goto bad_inode;
make_now:
	if (ino == F2FS_NODE_INO(sbi)) {
		inode->i_mapping->a_ops = &f2fs_node_aops;
		mapping_set_gfp_mask(inode->i_mapping, GFP_NOFS);
	} else if (ino == F2FS_META_INO(sbi)) {
		inode->i_mapping->a_ops = &f2fs_meta_aops;
		mapping_set_gfp_mask(inode->i_mapping, GFP_NOFS);
	} else if (ino == F2FS_COMPRESS_INO(sbi)) {
// #ifdef CONFIG_F2FS_FS_COMPRESSION
		inode->i_mapping->a_ops = &f2fs_compress_aops;
		/*
		 * generic_error_remove_folio only truncates pages of regular
		 * inode
		 */
		inode->i_mode |= S_IFREG;
#endif
		mapping_set_gfp_mask(inode->i_mapping,
			GFP_NOFS | __GFP_HIGHMEM | __GFP_MOVABLE);
	} else if (S_ISREG(inode->i_mode)) {
		inode->i_op = &f2fs_file_inode_operations;
		inode->i_fop = &f2fs_file_operations;
		inode->i_mapping->a_ops = &f2fs_dblock_aops;
	} else if (S_ISDIR(inode->i_mode)) {
		inode->i_op = &f2fs_dir_inode_operations;
		inode->i_fop = &f2fs_dir_operations;
		inode->i_mapping->a_ops = &f2fs_dblock_aops;
		mapping_set_gfp_mask(inode->i_mapping, GFP_NOFS);
	} else if (S_ISLNK(inode->i_mode)) {
		if (file_is_encrypt(inode))
			inode->i_op = &f2fs_encrypted_symlink_inode_operations;
		else
			inode->i_op = &f2fs_symlink_inode_operations;
		inode_nohighmem(inode);
		inode->i_mapping->a_ops = &f2fs_dblock_aops;
	} else if (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode) ||
			S_ISFIFO(inode->i_mode) || S_ISSOCK(inode->i_mode)) {
		inode->i_op = &f2fs_special_inode_operations;
		init_special_inode(inode, inode->i_mode, inode->i_rdev);
	} else {
		ret = -EIO;
		goto bad_inode;
	}
	f2fs_set_inode_flags(inode);

	if (file_should_truncate(inode) &&
			!is_sbi_flag_set(sbi, SBI_POR_DOING)) {
		ret = f2fs_truncate(inode);
		if (ret)
			goto bad_inode;
		file_dont_truncate(inode);
	}

	unlock_new_inode(inode);
	trace_f2fs_iget(inode);
	return inode;

bad_inode:
	f2fs_inode_synced(inode);
	iget_failed(inode);
	trace_f2fs_iget_exit(inode, ret);
	return ERR_PTR(ret);
}
