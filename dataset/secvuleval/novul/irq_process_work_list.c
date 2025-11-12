static void irq_process_work_list(struct idxd_irq_entry *irq_entry)
{
	LIST_HEAD(flist);
	struct idxd_desc *desc, *n;

	/*
	 * This lock protects list corruption from access of list outside of the irq handler
	 * thread.
	 */
	spin_lock(&irq_entry->list_lock);
	if (list_empty(&irq_entry->work_list)) {
		spin_unlock(&irq_entry->list_lock);
		return;
	}

	list_for_each_entry_safe(desc, n, &irq_entry->work_list, list) {
		if (desc->completion->status) {
			list_move_tail(&desc->list, &flist);
		}
	}

	spin_unlock(&irq_entry->list_lock);

	list_for_each_entry_safe(desc, n, &flist, list) {
		/*
		 * Check against the original status as ABORT is software defined
		 * and 0xff, which DSA_COMP_STATUS_MASK can mask out.
		 */
		list_del(&desc->list);

		if (unlikely(desc->completion->status == IDXD_COMP_DESC_ABORT)) {
			idxd_desc_complete(desc, IDXD_COMPLETE_ABORT, true);
			continue;
		}

		idxd_desc_complete(desc, IDXD_COMPLETE_NORMAL, true);
	}
}
