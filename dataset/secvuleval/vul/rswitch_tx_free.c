static void rswitch_tx_free(struct net_device *ndev)
{
	struct rswitch_device *rdev = netdev_priv(ndev);
	struct rswitch_gwca_queue *gq = rdev->tx_queue;
	struct rswitch_ext_desc *desc;
	struct sk_buff *skb;

	for (; rswitch_get_num_cur_queues(gq) > 0;
	     gq->dirty = rswitch_next_queue_index(gq, false, 1)) {
		desc = &gq->tx_ring[gq->dirty];
		if ((desc->desc.die_dt & DT_MASK) != DT_FEMPTY)
			break;

		dma_rmb();
		skb = gq->skbs[gq->dirty];
		if (skb) {
			dma_unmap_single(ndev->dev.parent,
					 gq->unmap_addrs[gq->dirty],
					 skb->len, DMA_TO_DEVICE);
			dev_kfree_skb_any(gq->skbs[gq->dirty]);
			gq->skbs[gq->dirty] = NULL;
			rdev->ndev->stats.tx_packets++;
			rdev->ndev->stats.tx_bytes += skb->len;
		}
		desc->desc.die_dt = DT_EEMPTY;
	}
}
