int folio_migrate_mapping(struct address_space *mapping,
		struct folio *newfolio, struct folio *folio, int extra_count)
{
	XA_STATE(xas, &mapping->i_pages, folio_index(folio));
	struct zone *oldzone, *newzone;
	int dirty;
	int expected_count = folio_expected_refs(mapping, folio) + extra_count;
	long nr = folio_nr_pages(folio);
	long entries, i;

	if (!mapping) {
		/* Anonymous page without mapping */
		if (folio_ref_count(folio) != expected_count)
			return -EAGAIN;

		/* Take off deferred split queue while frozen and memcg set */
		if (folio_test_large(folio) &&
		    folio_test_large_rmappable(folio)) {
			if (!folio_ref_freeze(folio, expected_count))
				return -EAGAIN;
			folio_undo_large_rmappable(folio);
			folio_ref_unfreeze(folio, expected_count);
		}

		/* No turning back from here */
		newfolio->index = folio->index;
		newfolio->mapping = folio->mapping;
		if (folio_test_swapbacked(folio))
			__folio_set_swapbacked(newfolio);

		return MIGRATEPAGE_SUCCESS;
	}

	oldzone = folio_zone(folio);
	newzone = folio_zone(newfolio);

	xas_lock_irq(&xas);
	if (!folio_ref_freeze(folio, expected_count)) {
		xas_unlock_irq(&xas);
		return -EAGAIN;
	}

	/* Take off deferred split queue while frozen and memcg set */
	if (folio_test_large(folio) && folio_test_large_rmappable(folio))
		folio_undo_large_rmappable(folio);

	/*
	 * Now we know that no one else is looking at the folio:
	 * no turning back from here.
	 */
	newfolio->index = folio->index;
	newfolio->mapping = folio->mapping;
	folio_ref_add(newfolio, nr); /* add cache reference */
	if (folio_test_swapbacked(folio)) {
		__folio_set_swapbacked(newfolio);
		if (folio_test_swapcache(folio)) {
			folio_set_swapcache(newfolio);
			newfolio->private = folio_get_private(folio);
		}
		entries = nr;
	} else {
		VM_BUG_ON_FOLIO(folio_test_swapcache(folio), folio);
		entries = 1;
	}

	/* Move dirty while page refs frozen and newpage not yet exposed */
	dirty = folio_test_dirty(folio);
	if (dirty) {
		folio_clear_dirty(folio);
		folio_set_dirty(newfolio);
	}

	/* Swap cache still stores N entries instead of a high-order entry */
	for (i = 0; i < entries; i++) {
		xas_store(&xas, newfolio);
		xas_next(&xas);
	}

	/*
	 * Drop cache reference from old page by unfreezing
	 * to one less reference.
	 * We know this isn't the last reference.
	 */
	folio_ref_unfreeze(folio, expected_count - nr);

	xas_unlock(&xas);
	/* Leave irq disabled to prevent preemption while updating stats */

	/*
	 * If moved to a different zone then also account
	 * the page for that zone. Other VM counters will be
	 * taken care of when we establish references to the
	 * new page and drop references to the old page.
	 *
	 * Note that anonymous pages are accounted for
	 * via NR_FILE_PAGES and NR_ANON_MAPPED if they
	 * are mapped to swap space.
	 */
	if (newzone != oldzone) {
		struct lruvec *old_lruvec, *new_lruvec;
		struct mem_cgroup *memcg;

		memcg = folio_memcg(folio);
		old_lruvec = mem_cgroup_lruvec(memcg, oldzone->zone_pgdat);
		new_lruvec = mem_cgroup_lruvec(memcg, newzone->zone_pgdat);

		__mod_lruvec_state(old_lruvec, NR_FILE_PAGES, -nr);
		__mod_lruvec_state(new_lruvec, NR_FILE_PAGES, nr);
		if (folio_test_swapbacked(folio) && !folio_test_swapcache(folio)) {
			__mod_lruvec_state(old_lruvec, NR_SHMEM, -nr);
			__mod_lruvec_state(new_lruvec, NR_SHMEM, nr);

			if (folio_test_pmd_mappable(folio)) {
				__mod_lruvec_state(old_lruvec, NR_SHMEM_THPS, -nr);
				__mod_lruvec_state(new_lruvec, NR_SHMEM_THPS, nr);
			}
		}
// #ifdef CONFIG_SWAP
		if (folio_test_swapcache(folio)) {
			__mod_lruvec_state(old_lruvec, NR_SWAPCACHE, -nr);
			__mod_lruvec_state(new_lruvec, NR_SWAPCACHE, nr);
		}
#endif
		if (dirty && mapping_can_writeback(mapping)) {
			__mod_lruvec_state(old_lruvec, NR_FILE_DIRTY, -nr);
			__mod_zone_page_state(oldzone, NR_ZONE_WRITE_PENDING, -nr);
			__mod_lruvec_state(new_lruvec, NR_FILE_DIRTY, nr);
			__mod_zone_page_state(newzone, NR_ZONE_WRITE_PENDING, nr);
		}
	}
	local_irq_enable();

	return MIGRATEPAGE_SUCCESS;
}
