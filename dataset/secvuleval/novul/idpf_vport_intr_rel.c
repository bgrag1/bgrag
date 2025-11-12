void idpf_vport_intr_rel(struct idpf_vport *vport)
{
	for (u32 v_idx = 0; v_idx < vport->num_q_vectors; v_idx++) {
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

	kfree(vport->q_vectors);
	vport->q_vectors = NULL;
}
