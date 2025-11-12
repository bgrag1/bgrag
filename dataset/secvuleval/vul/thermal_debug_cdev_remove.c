void thermal_debug_cdev_remove(struct thermal_cooling_device *cdev)
{
	struct thermal_debugfs *thermal_dbg = cdev->debugfs;

	if (!thermal_dbg)
		return;

	mutex_lock(&thermal_dbg->lock);

	thermal_debugfs_cdev_clear(&thermal_dbg->cdev_dbg);
	cdev->debugfs = NULL;

	mutex_unlock(&thermal_dbg->lock);

	thermal_debugfs_remove_id(thermal_dbg);
}
