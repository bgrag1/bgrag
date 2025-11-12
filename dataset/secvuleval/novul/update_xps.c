static int update_xps(struct dpaa2_eth_priv *priv)
{
	struct net_device *net_dev = priv->net_dev;
	int i, num_queues, netdev_queues;
	struct dpaa2_eth_fq *fq;
	cpumask_var_t xps_mask;
	int err = 0;

	if (!alloc_cpumask_var(&xps_mask, GFP_KERNEL))
		return -ENOMEM;

	num_queues = dpaa2_eth_queue_count(priv);
	netdev_queues = (net_dev->num_tc ? : 1) * num_queues;

	/* The first <num_queues> entries in priv->fq array are Tx/Tx conf
	 * queues, so only process those
	 */
	for (i = 0; i < netdev_queues; i++) {
		fq = &priv->fq[i % num_queues];

		cpumask_clear(xps_mask);
		cpumask_set_cpu(fq->target_cpu, xps_mask);

		err = netif_set_xps_queue(net_dev, xps_mask, i);
		if (err) {
			netdev_warn_once(net_dev, "Error setting XPS queue\n");
			break;
		}
	}

	free_cpumask_var(xps_mask);
	return err;
}
