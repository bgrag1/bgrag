struct ata_host *ata_host_alloc(struct device *dev, int max_ports)
{
	struct ata_host *host;
	size_t sz;
	int i;
	void *dr;

	/* alloc a container for our list of ATA ports (buses) */
	sz = sizeof(struct ata_host) + (max_ports + 1) * sizeof(void *);
	host = kzalloc(sz, GFP_KERNEL);
	if (!host)
		return NULL;

	if (!devres_open_group(dev, NULL, GFP_KERNEL))
		goto err_free;

	dr = devres_alloc(ata_devres_release, 0, GFP_KERNEL);
	if (!dr)
		goto err_out;

	devres_add(dev, dr);
	dev_set_drvdata(dev, host);

	spin_lock_init(&host->lock);
	mutex_init(&host->eh_mutex);
	host->dev = dev;
	host->n_ports = max_ports;
	kref_init(&host->kref);

	/* allocate ports bound to this host */
	for (i = 0; i < max_ports; i++) {
		struct ata_port *ap;

		ap = ata_port_alloc(host);
		if (!ap)
			goto err_out;

		ap->port_no = i;
		host->ports[i] = ap;
	}

	devres_remove_group(dev, NULL);
	return host;

 err_out:
	devres_release_group(dev, NULL);
 err_free:
	kfree(host);
	return NULL;
}
