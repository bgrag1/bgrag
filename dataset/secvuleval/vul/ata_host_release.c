static void ata_host_release(struct kref *kref)
{
	struct ata_host *host = container_of(kref, struct ata_host, kref);
	int i;

	for (i = 0; i < host->n_ports; i++) {
		struct ata_port *ap = host->ports[i];

		kfree(ap->pmp_link);
		kfree(ap->slave_link);
		kfree(ap->ncq_sense_buf);
		kfree(ap);
		host->ports[i] = NULL;
	}
	kfree(host);
}
