void padata_do_serial(struct padata_priv *padata)
{
	struct parallel_data *pd = padata->pd;
	int hashed_cpu = padata_cpu_hash(pd, padata->seq_nr);
	struct padata_list *reorder = per_cpu_ptr(pd->reorder_list, hashed_cpu);
	struct padata_priv *cur;
	struct list_head *pos;

	spin_lock(&reorder->lock);
	/* Sort in ascending order of sequence number. */
	list_for_each_prev(pos, &reorder->list) {
		cur = list_entry(pos, struct padata_priv, list);
		/* Compare by difference to consider integer wrap around */
		if ((signed int)(cur->seq_nr - padata->seq_nr) < 0)
			break;
	}
	list_add(&padata->list, pos);
	spin_unlock(&reorder->lock);

	/*
	 * Ensure the addition to the reorder list is ordered correctly
	 * with the trylock of pd->lock in padata_reorder.  Pairs with smp_mb
	 * in padata_reorder.
	 */
	smp_mb();

	padata_reorder(pd);
}
