static int f2fs_file_open(struct inode *inode, struct file *filp)
{
	int err = fscrypt_file_open(inode, filp);

	if (err)
		return err;

	if (!f2fs_is_compress_backend_ready(inode))
		return -EOPNOTSUPP;

	err = fsverity_file_open(inode, filp);
	if (err)
		return err;

	filp->f_mode |= FMODE_NOWAIT;
	filp->f_mode |= FMODE_CAN_ODIRECT;

	return dquot_file_open(inode, filp);
}
