static int am65_cpsw_ndo_xdp_xmit(struct net_device *ndev, int n,
				  struct xdp_frame **frames, u32 flags)
{
	struct am65_cpsw_tx_chn *tx_chn;
	struct netdev_queue *netif_txq;
	int cpu = smp_processor_id();
	int i, nxmit = 0;

	tx_chn = &am65_ndev_to_common(ndev)->tx_chns[cpu % AM65_CPSW_MAX_TX_QUEUES];
	netif_txq = netdev_get_tx_queue(ndev, tx_chn->id);

	__netif_tx_lock(netif_txq, cpu);
	for (i = 0; i < n; i++) {
		if (am65_cpsw_xdp_tx_frame(ndev, tx_chn, frames[i],
					   AM65_CPSW_TX_BUF_TYPE_XDP_NDO))
			break;
		nxmit++;
	}
	__netif_tx_unlock(netif_txq);

	return nxmit;
}
