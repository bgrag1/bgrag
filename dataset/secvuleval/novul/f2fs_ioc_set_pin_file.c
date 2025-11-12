static int f2fs_ioc_set_pin_file(struct file *filp, unsigned long arg)
{
	struct inode *inode = file_inode(filp);
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	__u32 pin;
	int ret = 0;

	if (get_user(pin, (__u32 __user *)arg))
		return -EFAULT;

	if (!S_ISREG(inode->i_mode))
		return -EINVAL;

	if (f2fs_readonly(sbi->sb))
		return -EROFS;

	ret = mnt_want_write_file(filp);
	if (ret)
		return ret;

	inode_lock(inode);

	if (f2fs_is_atomic_file(inode)) {
		ret = -EINVAL;
		goto out;
	}

	if (!pin) {
		clear_inode_flag(inode, FI_PIN_FILE);
		f2fs_i_gc_failures_write(inode, 0);
		goto done;
	} else if (f2fs_is_pinned_file(inode)) {
		goto done;
	}

	if (F2FS_HAS_BLOCKS(inode)) {
		ret = -EFBIG;
		goto out;
	}

	/* Let's allow file pinning on zoned device. */
	if (!f2fs_sb_has_blkzoned(sbi) &&
	    f2fs_should_update_outplace(inode, NULL)) {
		ret = -EINVAL;
		goto out;
	}

	if (f2fs_pin_file_control(inode, false)) {
		ret = -EAGAIN;
		goto out;
	}

	ret = f2fs_convert_inline_inode(inode);
	if (ret)
		goto out;

	if (!f2fs_disable_compressed_file(inode)) {
		ret = -EOPNOTSUPP;
		goto out;
	}

	set_inode_flag(inode, FI_PIN_FILE);
	ret = F2FS_I(inode)->i_gc_failures;
done:
	f2fs_update_time(sbi, REQ_TIME);
out:
	inode_unlock(inode);
	mnt_drop_write_file(filp);
	return ret;
}
