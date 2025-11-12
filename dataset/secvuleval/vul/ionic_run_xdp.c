static bool ionic_run_xdp(struct ionic_rx_stats *stats,
			  struct net_device *netdev,
			  struct bpf_prog *xdp_prog,
			  struct ionic_queue *rxq,
			  struct ionic_buf_info *buf_info,
			  int len)
{
	u32 xdp_action = XDP_ABORTED;
	struct xdp_buff xdp_buf;
	struct ionic_queue *txq;
	struct netdev_queue *nq;
	struct xdp_frame *xdpf;
	int remain_len;
	int frag_len;
	int err = 0;

	xdp_init_buff(&xdp_buf, IONIC_PAGE_SIZE, rxq->xdp_rxq_info);
	frag_len = min_t(u16, len, IONIC_XDP_MAX_LINEAR_MTU + VLAN_ETH_HLEN);
	xdp_prepare_buff(&xdp_buf, ionic_rx_buf_va(buf_info),
			 XDP_PACKET_HEADROOM, frag_len, false);

	dma_sync_single_range_for_cpu(rxq->dev, ionic_rx_buf_pa(buf_info),
				      XDP_PACKET_HEADROOM, len,
				      DMA_FROM_DEVICE);

	prefetchw(&xdp_buf.data_hard_start);

	/*  We limit MTU size to one buffer if !xdp_has_frags, so
	 *  if the recv len is bigger than one buffer
	 *     then we know we have frag info to gather
	 */
	remain_len = len - frag_len;
	if (remain_len) {
		struct skb_shared_info *sinfo;
		struct ionic_buf_info *bi;
		skb_frag_t *frag;

		bi = buf_info;
		sinfo = xdp_get_shared_info_from_buff(&xdp_buf);
		sinfo->nr_frags = 0;
		sinfo->xdp_frags_size = 0;
		xdp_buff_set_frags_flag(&xdp_buf);

		do {
			if (unlikely(sinfo->nr_frags >= MAX_SKB_FRAGS)) {
				err = -ENOSPC;
				goto out_xdp_abort;
			}

			frag = &sinfo->frags[sinfo->nr_frags];
			sinfo->nr_frags++;
			bi++;
			frag_len = min_t(u16, remain_len, ionic_rx_buf_size(bi));
			dma_sync_single_range_for_cpu(rxq->dev, ionic_rx_buf_pa(bi),
						      0, frag_len, DMA_FROM_DEVICE);
			skb_frag_fill_page_desc(frag, bi->page, 0, frag_len);
			sinfo->xdp_frags_size += frag_len;
			remain_len -= frag_len;

			if (page_is_pfmemalloc(bi->page))
				xdp_buff_set_frag_pfmemalloc(&xdp_buf);
		} while (remain_len > 0);
	}

	xdp_action = bpf_prog_run_xdp(xdp_prog, &xdp_buf);

	switch (xdp_action) {
	case XDP_PASS:
		stats->xdp_pass++;
		return false;  /* false = we didn't consume the packet */

	case XDP_DROP:
		ionic_rx_page_free(rxq, buf_info);
		stats->xdp_drop++;
		break;

	case XDP_TX:
		xdpf = xdp_convert_buff_to_frame(&xdp_buf);
		if (!xdpf)
			goto out_xdp_abort;

		txq = rxq->partner;
		nq = netdev_get_tx_queue(netdev, txq->index);
		__netif_tx_lock(nq, smp_processor_id());
		txq_trans_cond_update(nq);

		if (netif_tx_queue_stopped(nq) ||
		    !netif_txq_maybe_stop(q_to_ndq(netdev, txq),
					  ionic_q_space_avail(txq),
					  1, 1)) {
			__netif_tx_unlock(nq);
			goto out_xdp_abort;
		}

		dma_unmap_page(rxq->dev, buf_info->dma_addr,
			       IONIC_PAGE_SIZE, DMA_FROM_DEVICE);

		err = ionic_xdp_post_frame(txq, xdpf, XDP_TX,
					   buf_info->page,
					   buf_info->page_offset,
					   true);
		__netif_tx_unlock(nq);
		if (err) {
			netdev_dbg(netdev, "tx ionic_xdp_post_frame err %d\n", err);
			goto out_xdp_abort;
		}
		stats->xdp_tx++;

		/* the Tx completion will free the buffers */
		break;

	case XDP_REDIRECT:
		/* unmap the pages before handing them to a different device */
		dma_unmap_page(rxq->dev, buf_info->dma_addr,
			       IONIC_PAGE_SIZE, DMA_FROM_DEVICE);

		err = xdp_do_redirect(netdev, &xdp_buf, xdp_prog);
		if (err) {
			netdev_dbg(netdev, "xdp_do_redirect err %d\n", err);
			goto out_xdp_abort;
		}
		buf_info->page = NULL;
		rxq->xdp_flush = true;
		stats->xdp_redirect++;
		break;

	case XDP_ABORTED:
	default:
		goto out_xdp_abort;
	}

	return true;

out_xdp_abort:
	trace_xdp_exception(netdev, xdp_prog, xdp_action);
	ionic_rx_page_free(rxq, buf_info);
	stats->xdp_aborted++;

	return true;
}
