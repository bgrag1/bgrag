int stm_register_device(struct device *parent, struct stm_data *stm_data,
			struct module *owner)
{
	struct stm_device *stm;
	unsigned int nmasters;
	int err = -ENOMEM;

	if (!stm_core_up)
		return -EPROBE_DEFER;

	if (!stm_data->packet || !stm_data->sw_nchannels)
		return -EINVAL;

	nmasters = stm_data->sw_end - stm_data->sw_start + 1;
	stm = vzalloc(sizeof(*stm) + nmasters * sizeof(void *));
	if (!stm)
		return -ENOMEM;

	stm->major = register_chrdev(0, stm_data->name, &stm_fops);
	if (stm->major < 0)
		goto err_free;

	device_initialize(&stm->dev);
	stm->dev.devt = MKDEV(stm->major, 0);
	stm->dev.class = &stm_class;
	stm->dev.parent = parent;
	stm->dev.release = stm_device_release;

	mutex_init(&stm->link_mutex);
	spin_lock_init(&stm->link_lock);
	INIT_LIST_HEAD(&stm->link_list);

	/* initialize the object before it is accessible via sysfs */
	spin_lock_init(&stm->mc_lock);
	mutex_init(&stm->policy_mutex);
	stm->sw_nmasters = nmasters;
	stm->owner = owner;
	stm->data = stm_data;
	stm_data->stm = stm;

	err = kobject_set_name(&stm->dev.kobj, "%s", stm_data->name);
	if (err)
		goto err_device;

	err = device_add(&stm->dev);
	if (err)
		goto err_device;

	/*
	 * Use delayed autosuspend to avoid bouncing back and forth
	 * on recurring character device writes, with the initial
	 * delay time of 2 seconds.
	 */
	pm_runtime_no_callbacks(&stm->dev);
	pm_runtime_use_autosuspend(&stm->dev);
	pm_runtime_set_autosuspend_delay(&stm->dev, 2000);
	pm_runtime_set_suspended(&stm->dev);
	pm_runtime_enable(&stm->dev);

	return 0;

err_device:
	unregister_chrdev(stm->major, stm_data->name);

	/* matches device_initialize() above */
	put_device(&stm->dev);
err_free:
	vfree(stm);

	return err;
}
