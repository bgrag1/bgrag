static int cdrom_ioctl_timed_media_change(struct cdrom_device_info *cdi,
		unsigned long arg)
{
	int ret;
	struct cdrom_timed_media_change_info __user *info;
	struct cdrom_timed_media_change_info tmp_info;

	if (!CDROM_CAN(CDC_MEDIA_CHANGED))
		return -ENOSYS;

	info = (struct cdrom_timed_media_change_info __user *)arg;
	cd_dbg(CD_DO_IOCTL, "entering CDROM_TIMED_MEDIA_CHANGE\n");

	ret = cdrom_ioctl_media_changed(cdi, CDSL_CURRENT);
	if (ret < 0)
		return ret;

	if (copy_from_user(&tmp_info, info, sizeof(tmp_info)) != 0)
		return -EFAULT;

	tmp_info.media_flags = 0;
	if (cdi->last_media_change_ms > tmp_info.last_media_change)
		tmp_info.media_flags |= MEDIA_CHANGED_FLAG;

	tmp_info.last_media_change = cdi->last_media_change_ms;

	if (copy_to_user(info, &tmp_info, sizeof(*info)) != 0)
		return -EFAULT;

	return 0;
}
