long kvm_spapr_tce_attach_iommu_group(struct kvm *kvm, int tablefd,
				      struct iommu_group *grp)
{
	struct kvmppc_spapr_tce_table *stt = NULL;
	bool found = false;
	struct iommu_table *tbl = NULL;
	struct iommu_table_group *table_group;
	long i;
	struct kvmppc_spapr_tce_iommu_table *stit;
	struct fd f;

	f = fdget(tablefd);
	if (!f.file)
		return -EBADF;

	rcu_read_lock();
	list_for_each_entry_rcu(stt, &kvm->arch.spapr_tce_tables, list) {
		if (stt == f.file->private_data) {
			found = true;
			break;
		}
	}
	rcu_read_unlock();

	fdput(f);

	if (!found)
		return -EINVAL;

	table_group = iommu_group_get_iommudata(grp);
	if (WARN_ON(!table_group))
		return -EFAULT;

	for (i = 0; i < IOMMU_TABLE_GROUP_MAX_TABLES; ++i) {
		struct iommu_table *tbltmp = table_group->tables[i];

		if (!tbltmp)
			continue;
		/* Make sure hardware table parameters are compatible */
		if ((tbltmp->it_page_shift <= stt->page_shift) &&
				(tbltmp->it_offset << tbltmp->it_page_shift ==
				 stt->offset << stt->page_shift) &&
				(tbltmp->it_size << tbltmp->it_page_shift >=
				 stt->size << stt->page_shift)) {
			/*
			 * Reference the table to avoid races with
			 * add/remove DMA windows.
			 */
			tbl = iommu_tce_table_get(tbltmp);
			break;
		}
	}
	if (!tbl)
		return -EINVAL;

	rcu_read_lock();
	list_for_each_entry_rcu(stit, &stt->iommu_tables, next) {
		if (tbl != stit->tbl)
			continue;

		if (!kref_get_unless_zero(&stit->kref)) {
			/* stit is being destroyed */
			iommu_tce_table_put(tbl);
			rcu_read_unlock();
			return -ENOTTY;
		}
		/*
		 * The table is already known to this KVM, we just increased
		 * its KVM reference counter and can return.
		 */
		rcu_read_unlock();
		return 0;
	}
	rcu_read_unlock();

	stit = kzalloc(sizeof(*stit), GFP_KERNEL);
	if (!stit) {
		iommu_tce_table_put(tbl);
		return -ENOMEM;
	}

	stit->tbl = tbl;
	kref_init(&stit->kref);

	list_add_rcu(&stit->next, &stt->iommu_tables);

	return 0;
}
