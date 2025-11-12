static void
ltq_etop_free_channel(struct net_device *dev, struct ltq_etop_chan *ch)
{
	struct ltq_etop_priv *priv = netdev_priv(dev);

	ltq_dma_free(&ch->dma);
	if (ch->dma.irq)
		free_irq(ch->dma.irq, priv);
	if (IS_RX(ch->idx)) {
		struct ltq_dma_channel *dma = &ch->dma;

		for (dma->desc = 0; dma->desc < LTQ_DESC_NUM; dma->desc++)
			dev_kfree_skb_any(ch->skb[ch->dma.desc]);
	}
}
