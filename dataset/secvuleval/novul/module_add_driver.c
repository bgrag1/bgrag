int module_add_driver(struct module *mod, const struct device_driver *drv)
{
	char *driver_name;
	struct module_kobject *mk = NULL;
	int ret;

	if (!drv)
		return 0;

	if (mod)
		mk = &mod->mkobj;
	else if (drv->mod_name) {
		struct kobject *mkobj;

		/* Lookup built-in module entry in /sys/modules */
		mkobj = kset_find_obj(module_kset, drv->mod_name);
		if (mkobj) {
			mk = container_of(mkobj, struct module_kobject, kobj);
			/* remember our module structure */
			drv->p->mkobj = mk;
			/* kset_find_obj took a reference */
			kobject_put(mkobj);
		}
	}

	if (!mk)
		return 0;

	ret = sysfs_create_link(&drv->p->kobj, &mk->kobj, "module");
	if (ret)
		return ret;

	driver_name = make_driver_name(drv);
	if (!driver_name) {
		ret = -ENOMEM;
		goto out_remove_kobj;
	}

	module_create_drivers_dir(mk);
	if (!mk->drivers_dir) {
		ret = -EINVAL;
		goto out_free_driver_name;
	}

	ret = sysfs_create_link(mk->drivers_dir, &drv->p->kobj, driver_name);
	if (ret)
		goto out_remove_drivers_dir;

	kfree(driver_name);

	return 0;

out_remove_drivers_dir:
	sysfs_remove_link(mk->drivers_dir, driver_name);

out_free_driver_name:
	kfree(driver_name);

out_remove_kobj:
	sysfs_remove_link(&drv->p->kobj, "module");
	return ret;
}
