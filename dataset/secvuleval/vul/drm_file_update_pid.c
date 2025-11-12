void drm_file_update_pid(struct drm_file *filp)
{
	struct drm_device *dev;
	struct pid *pid, *old;

	/*
	 * Master nodes need to keep the original ownership in order for
	 * drm_master_check_perm to keep working correctly. (See comment in
	 * drm_auth.c.)
	 */
	if (filp->was_master)
		return;

	pid = task_tgid(current);

	/*
	 * Quick unlocked check since the model is a single handover followed by
	 * exclusive repeated use.
	 */
	if (pid == rcu_access_pointer(filp->pid))
		return;

	dev = filp->minor->dev;
	mutex_lock(&dev->filelist_mutex);
	old = rcu_replace_pointer(filp->pid, pid, 1);
	mutex_unlock(&dev->filelist_mutex);

	if (pid != old) {
		get_pid(pid);
		synchronize_rcu();
		put_pid(old);
	}
}
