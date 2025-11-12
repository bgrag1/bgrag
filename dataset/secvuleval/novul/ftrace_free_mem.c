void ftrace_free_mem(struct module *mod, void *start_ptr, void *end_ptr)
{
	unsigned long start = (unsigned long)(start_ptr);
	unsigned long end = (unsigned long)(end_ptr);
	struct ftrace_page **last_pg = &ftrace_pages_start;
	struct ftrace_page *tmp_page = NULL;
	struct ftrace_page *pg;
	struct dyn_ftrace *rec;
	struct dyn_ftrace key;
	struct ftrace_mod_map *mod_map = NULL;
	struct ftrace_init_func *func, *func_next;
	LIST_HEAD(clear_hash);

	key.ip = start;
	key.flags = end;	/* overload flags, as it is unsigned long */

	mutex_lock(&ftrace_lock);

	/*
	 * If we are freeing module init memory, then check if
	 * any tracer is active. If so, we need to save a mapping of
	 * the module functions being freed with the address.
	 */
	if (mod && ftrace_ops_list != &ftrace_list_end)
		mod_map = allocate_ftrace_mod_map(mod, start, end);

	for (pg = ftrace_pages_start; pg; last_pg = &pg->next, pg = *last_pg) {
		if (end < pg->records[0].ip ||
		    start >= (pg->records[pg->index - 1].ip + MCOUNT_INSN_SIZE))
			continue;
 again:
		rec = bsearch(&key, pg->records, pg->index,
			      sizeof(struct dyn_ftrace),
			      ftrace_cmp_recs);
		if (!rec)
			continue;

		/* rec will be cleared from hashes after ftrace_lock unlock */
		add_to_clear_hash_list(&clear_hash, rec);

		if (mod_map)
			save_ftrace_mod_rec(mod_map, rec);

		pg->index--;
		ftrace_update_tot_cnt--;
		if (!pg->index) {
			*last_pg = pg->next;
			pg->next = tmp_page;
			tmp_page = pg;
			pg = container_of(last_pg, struct ftrace_page, next);
			if (!(*last_pg))
				ftrace_pages = pg;
			continue;
		}
		memmove(rec, rec + 1,
			(pg->index - (rec - pg->records)) * sizeof(*rec));
		/* More than one function may be in this block */
		goto again;
	}
	mutex_unlock(&ftrace_lock);

	list_for_each_entry_safe(func, func_next, &clear_hash, list) {
		clear_func_from_hashes(func);
		kfree(func);
	}
	/* Need to synchronize with ftrace_location_range() */
	if (tmp_page) {
		synchronize_rcu();
		ftrace_free_pages(tmp_page);
	}
}
