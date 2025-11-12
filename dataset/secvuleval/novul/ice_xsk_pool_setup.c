int ice_xsk_pool_setup(struct ice_vsi *vsi, struct xsk_buff_pool *pool, u16 qid)
{
	bool if_running, pool_present = !!pool;
	int ret = 0, pool_failure = 0;

	if (qid >= vsi->num_rxq || qid >= vsi->num_txq) {
		netdev_err(vsi->netdev, "Please use queue id in scope of combined queues count\n");
		pool_failure = -EINVAL;
		goto failure;
	}

	if_running = !test_bit(ICE_VSI_DOWN, vsi->state) &&
		     ice_is_xdp_ena_vsi(vsi);

	if (if_running) {
		struct ice_rx_ring *rx_ring = vsi->rx_rings[qid];

		ret = ice_qp_dis(vsi, qid);
		if (ret) {
			netdev_err(vsi->netdev, "ice_qp_dis error = %d\n", ret);
			goto xsk_pool_if_up;
		}

		ret = ice_realloc_rx_xdp_bufs(rx_ring, pool_present);
		if (ret)
			goto xsk_pool_if_up;
	}

	pool_failure = pool_present ? ice_xsk_pool_enable(vsi, pool, qid) :
				      ice_xsk_pool_disable(vsi, qid);

xsk_pool_if_up:
	if (if_running) {
		ret = ice_qp_ena(vsi, qid);
		if (!ret && pool_present)
			napi_schedule(&vsi->rx_rings[qid]->xdp_ring->q_vector->napi);
		else if (ret)
			netdev_err(vsi->netdev, "ice_qp_ena error = %d\n", ret);
	}

failure:
	if (pool_failure) {
		netdev_err(vsi->netdev, "Could not %sable buffer pool, error = %d\n",
			   pool_present ? "en" : "dis", pool_failure);
		return pool_failure;
	}

	return ret;
}
