void mem_cgroup_migrate(struct folio *old, struct folio *new)
{
	struct mem_cgroup *memcg;

	VM_BUG_ON_FOLIO(!folio_test_locked(old), old);
	VM_BUG_ON_FOLIO(!folio_test_locked(new), new);
	VM_BUG_ON_FOLIO(folio_test_anon(old) != folio_test_anon(new), new);
	VM_BUG_ON_FOLIO(folio_nr_pages(old) != folio_nr_pages(new), new);

	if (mem_cgroup_disabled())
		return;

	memcg = folio_memcg(old);
	/*
	 * Note that it is normal to see !memcg for a hugetlb folio.
	 * For e.g, itt could have been allocated when memory_hugetlb_accounting
	 * was not selected.
	 */
	VM_WARN_ON_ONCE_FOLIO(!folio_test_hugetlb(old) && !memcg, old);
	if (!memcg)
		return;

	/* Transfer the charge and the css ref */
	commit_charge(new, memcg);
	old->memcg_data = 0;
}
