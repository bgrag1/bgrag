static void ice_free_q_vector(struct ice_vsi *vsi, int v_idx)
{
	struct ice_q_vector *q_vector;
	struct ice_pf *pf = vsi->back;
	struct ice_tx_ring *tx_ring;
	struct ice_rx_ring *rx_ring;
	struct device *dev;

	dev = ice_pf_to_dev(pf);
	if (!vsi->q_vectors[v_idx]) {
		dev_dbg(dev, "Queue vector at index %d not found\n", v_idx);
		return;
	}
	q_vector = vsi->q_vectors[v_idx];

	ice_for_each_tx_ring(tx_ring, q_vector->tx) {
		ice_queue_set_napi(vsi, tx_ring->q_index, NETDEV_QUEUE_TYPE_TX,
				   NULL);
		tx_ring->q_vector = NULL;
	}
	ice_for_each_rx_ring(rx_ring, q_vector->rx) {
		ice_queue_set_napi(vsi, rx_ring->q_index, NETDEV_QUEUE_TYPE_RX,
				   NULL);
		rx_ring->q_vector = NULL;
	}

	/* only VSI with an associated netdev is set up with NAPI */
	if (vsi->netdev)
		netif_napi_del(&q_vector->napi);

	/* release MSIX interrupt if q_vector had interrupt allocated */
	if (q_vector->irq.index < 0)
		goto free_q_vector;

	/* only free last VF ctrl vsi interrupt */
	if (vsi->type == ICE_VSI_CTRL && vsi->vf &&
	    ice_get_vf_ctrl_vsi(pf, vsi))
		goto free_q_vector;

	ice_free_irq(pf, q_vector->irq);

free_q_vector:
	kfree(q_vector);
	vsi->q_vectors[v_idx] = NULL;
}
