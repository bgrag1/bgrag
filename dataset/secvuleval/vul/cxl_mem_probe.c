static int cxl_mem_probe(struct device *dev)
{
	struct cxl_memdev *cxlmd = to_cxl_memdev(dev);
	struct cxl_memdev_state *mds = to_cxl_memdev_state(cxlmd->cxlds);
	struct cxl_dev_state *cxlds = cxlmd->cxlds;
	struct device *endpoint_parent;
	struct cxl_port *parent_port;
	struct cxl_dport *dport;
	struct dentry *dentry;
	int rc;

	if (!cxlds->media_ready)
		return -EBUSY;

	/*
	 * Someone is trying to reattach this device after it lost its port
	 * connection (an endpoint port previously registered by this memdev was
	 * disabled). This racy check is ok because if the port is still gone,
	 * no harm done, and if the port hierarchy comes back it will re-trigger
	 * this probe. Port rescan and memdev detach work share the same
	 * single-threaded workqueue.
	 */
	if (work_pending(&cxlmd->detach_work))
		return -EBUSY;

	dentry = cxl_debugfs_create_dir(dev_name(dev));
	debugfs_create_devm_seqfile(dev, "dpamem", dentry, cxl_mem_dpa_show);

	if (test_bit(CXL_POISON_ENABLED_INJECT, mds->poison.enabled_cmds))
		debugfs_create_file("inject_poison", 0200, dentry, cxlmd,
				    &cxl_poison_inject_fops);
	if (test_bit(CXL_POISON_ENABLED_CLEAR, mds->poison.enabled_cmds))
		debugfs_create_file("clear_poison", 0200, dentry, cxlmd,
				    &cxl_poison_clear_fops);

	rc = devm_add_action_or_reset(dev, remove_debugfs, dentry);
	if (rc)
		return rc;

	rc = devm_cxl_enumerate_ports(cxlmd);
	if (rc)
		return rc;

	parent_port = cxl_mem_find_port(cxlmd, &dport);
	if (!parent_port) {
		dev_err(dev, "CXL port topology not found\n");
		return -ENXIO;
	}

	if (dport->rch)
		endpoint_parent = parent_port->uport_dev;
	else
		endpoint_parent = &parent_port->dev;

	cxl_setup_parent_dport(dev, dport);

	device_lock(endpoint_parent);
	if (!endpoint_parent->driver) {
		dev_err(dev, "CXL port topology %s not enabled\n",
			dev_name(endpoint_parent));
		rc = -ENXIO;
		goto unlock;
	}

	rc = devm_cxl_add_endpoint(endpoint_parent, cxlmd, dport);
unlock:
	device_unlock(endpoint_parent);
	put_device(&parent_port->dev);
	if (rc)
		return rc;

	if (resource_size(&cxlds->pmem_res) && IS_ENABLED(CONFIG_CXL_PMEM)) {
		rc = devm_cxl_add_nvdimm(cxlmd);
		if (rc == -ENODEV)
			dev_info(dev, "PMEM disabled by platform\n");
		else
			return rc;
	}

	/*
	 * The kernel may be operating out of CXL memory on this device,
	 * there is no spec defined way to determine whether this device
	 * preserves contents over suspend, and there is no simple way
	 * to arrange for the suspend image to avoid CXL memory which
	 * would setup a circular dependency between PCI resume and save
	 * state restoration.
	 *
	 * TODO: support suspend when all the regions this device is
	 * hosting are locked and covered by the system address map,
	 * i.e. platform firmware owns restoring the HDM configuration
	 * that it locked.
	 */
	cxl_mem_active_inc();
	return devm_add_action_or_reset(dev, enable_suspend, NULL);
}
