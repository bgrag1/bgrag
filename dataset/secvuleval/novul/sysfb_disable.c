void sysfb_disable(struct device *dev)
{
	struct screen_info *si = &screen_info;

	mutex_lock(&disable_lock);
	if (!dev || dev == sysfb_parent_dev(si)) {
		sysfb_unregister();
		disabled = true;
	}
	mutex_unlock(&disable_lock);
}
