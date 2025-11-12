void idpf_vport_intr_rel(struct idpf_vport *vport)
{
	int i, j, v_idx;

	for (v_idx = 0; v_idx < vport->num_q_vectors; v_idx++) {
		struct idpf_q_vector *q_vector = &vport->q_vectors[v_idx];

		kfree(q_vector->complq);
		q_vector->complq = NULL;
		kfree(q_vector->bufq);
		q_vector->bufq = NULL;
		kfree(q_vector->tx);
		q_vector->tx = NULL;
		kfree(q_vector->rx);
		q_vector->rx = NULL;

		free_cpumask_var(q_vector->affinity_mask);
	}

	/* Clean up the mapping of queues to vectors */
	for (i = 0; i < vport->num_rxq_grp; i++) {
		struct idpf_rxq_group *rx_qgrp = &vport->rxq_grps[i];

		if (idpf_is_queue_model_split(vport->rxq_model))
			for (j = 0; j < rx_qgrp->splitq.num_rxq_sets; j++)
				rx_qgrp->splitq.rxq_sets[j]->rxq.q_vector = NULL;
		else
			for (j = 0; j < rx_qgrp->singleq.num_rxq; j++)
				rx_qgrp->singleq.rxqs[j]->q_vector = NULL;
	}

	if (idpf_is_queue_model_split(vport->txq_model))
		for (i = 0; i < vport->num_txq_grp; i++)
			vport->txq_grps[i].complq->q_vector = NULL;
	else
		for (i = 0; i < vport->num_txq_grp; i++)
			for (j = 0; j < vport->txq_grps[i].num_txq; j++)
				vport->txq_grps[i].txqs[j]->q_vector = NULL;

	kfree(vport->q_vectors);
	vport->q_vectors = NULL;
}
