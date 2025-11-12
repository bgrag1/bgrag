static int f2fs_move_file_range(struct file *file_in, loff_t pos_in,
			struct file *file_out, loff_t pos_out, size_t len)
{
	struct inode *src = file_inode(file_in);
	struct inode *dst = file_inode(file_out);
	struct f2fs_sb_info *sbi = F2FS_I_SB(src);
	size_t olen = len, dst_max_i_size = 0;
	size_t dst_osize;
	int ret;

	if (file_in->f_path.mnt != file_out->f_path.mnt ||
				src->i_sb != dst->i_sb)
		return -EXDEV;

	if (unlikely(f2fs_readonly(src->i_sb)))
		return -EROFS;

	if (!S_ISREG(src->i_mode) || !S_ISREG(dst->i_mode))
		return -EINVAL;

	if (IS_ENCRYPTED(src) || IS_ENCRYPTED(dst))
		return -EOPNOTSUPP;

	if (pos_out < 0 || pos_in < 0)
		return -EINVAL;

	if (src == dst) {
		if (pos_in == pos_out)
			return 0;
		if (pos_out > pos_in && pos_out < pos_in + len)
			return -EINVAL;
	}

	inode_lock(src);
	if (src != dst) {
		ret = -EBUSY;
		if (!inode_trylock(dst))
			goto out;
	}

	if (f2fs_compressed_file(src) || f2fs_compressed_file(dst) ||
		f2fs_is_pinned_file(src) || f2fs_is_pinned_file(dst)) {
		ret = -EOPNOTSUPP;
		goto out_unlock;
	}

	if (f2fs_is_atomic_file(src) || f2fs_is_atomic_file(dst)) {
		ret = -EINVAL;
		goto out_unlock;
	}

	ret = -EINVAL;
	if (pos_in + len > src->i_size || pos_in + len < pos_in)
		goto out_unlock;
	if (len == 0)
		olen = len = src->i_size - pos_in;
	if (pos_in + len == src->i_size)
		len = ALIGN(src->i_size, F2FS_BLKSIZE) - pos_in;
	if (len == 0) {
		ret = 0;
		goto out_unlock;
	}

	dst_osize = dst->i_size;
	if (pos_out + olen > dst->i_size)
		dst_max_i_size = pos_out + olen;

	/* verify the end result is block aligned */
	if (!IS_ALIGNED(pos_in, F2FS_BLKSIZE) ||
			!IS_ALIGNED(pos_in + len, F2FS_BLKSIZE) ||
			!IS_ALIGNED(pos_out, F2FS_BLKSIZE))
		goto out_unlock;

	ret = f2fs_convert_inline_inode(src);
	if (ret)
		goto out_unlock;

	ret = f2fs_convert_inline_inode(dst);
	if (ret)
		goto out_unlock;

	/* write out all dirty pages from offset */
	ret = filemap_write_and_wait_range(src->i_mapping,
					pos_in, pos_in + len);
	if (ret)
		goto out_unlock;

	ret = filemap_write_and_wait_range(dst->i_mapping,
					pos_out, pos_out + len);
	if (ret)
		goto out_unlock;

	f2fs_balance_fs(sbi, true);

	f2fs_down_write(&F2FS_I(src)->i_gc_rwsem[WRITE]);
	if (src != dst) {
		ret = -EBUSY;
		if (!f2fs_down_write_trylock(&F2FS_I(dst)->i_gc_rwsem[WRITE]))
			goto out_src;
	}

	f2fs_lock_op(sbi);
	ret = __exchange_data_block(src, dst, F2FS_BYTES_TO_BLK(pos_in),
				F2FS_BYTES_TO_BLK(pos_out),
				F2FS_BYTES_TO_BLK(len), false);

	if (!ret) {
		if (dst_max_i_size)
			f2fs_i_size_write(dst, dst_max_i_size);
		else if (dst_osize != dst->i_size)
			f2fs_i_size_write(dst, dst_osize);
	}
	f2fs_unlock_op(sbi);

	if (src != dst)
		f2fs_up_write(&F2FS_I(dst)->i_gc_rwsem[WRITE]);
out_src:
	f2fs_up_write(&F2FS_I(src)->i_gc_rwsem[WRITE]);
	if (ret)
		goto out_unlock;

	inode_set_mtime_to_ts(src, inode_set_ctime_current(src));
	f2fs_mark_inode_dirty_sync(src, false);
	if (src != dst) {
		inode_set_mtime_to_ts(dst, inode_set_ctime_current(dst));
		f2fs_mark_inode_dirty_sync(dst, false);
	}
	f2fs_update_time(sbi, REQ_TIME);

out_unlock:
	if (src != dst)
		inode_unlock(dst);
out:
	inode_unlock(src);
	return ret;
}
