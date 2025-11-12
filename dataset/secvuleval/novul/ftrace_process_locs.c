static int ftrace_process_locs(struct module *mod,
			       unsigned long *start,
			       unsigned long *end)
{
	struct ftrace_page *pg_unuse = NULL;
	struct ftrace_page *start_pg;
	struct ftrace_page *pg;
	struct dyn_ftrace *rec;
	unsigned long skipped = 0;
	unsigned long count;
	unsigned long *p;
	unsigned long addr;
	unsigned long flags = 0; /* Shut up gcc */
	int ret = -ENOMEM;

	count = end - start;

	if (!count)
		return 0;

	/*
	 * Sorting mcount in vmlinux at build time depend on
	 * CONFIG_BUILDTIME_MCOUNT_SORT, while mcount loc in
	 * modules can not be sorted at build time.
	 */
	if (!IS_ENABLED(CONFIG_BUILDTIME_MCOUNT_SORT) || mod) {
		sort(start, count, sizeof(*start),
		     ftrace_cmp_ips, NULL);
	} else {
		test_is_sorted(start, count);
	}

	start_pg = ftrace_allocate_pages(count);
	if (!start_pg)
		return -ENOMEM;

	mutex_lock(&ftrace_lock);

	/*
	 * Core and each module needs their own pages, as
	 * modules will free them when they are removed.
	 * Force a new page to be allocated for modules.
	 */
	if (!mod) {
		WARN_ON(ftrace_pages || ftrace_pages_start);
		/* First initialization */
		ftrace_pages = ftrace_pages_start = start_pg;
	} else {
		if (!ftrace_pages)
			goto out;

		if (WARN_ON(ftrace_pages->next)) {
			/* Hmm, we have free pages? */
			while (ftrace_pages->next)
				ftrace_pages = ftrace_pages->next;
		}

		ftrace_pages->next = start_pg;
	}

	p = start;
	pg = start_pg;
	while (p < end) {
		unsigned long end_offset;
		addr = ftrace_call_adjust(*p++);
		/*
		 * Some architecture linkers will pad between
		 * the different mcount_loc sections of different
		 * object files to satisfy alignments.
		 * Skip any NULL pointers.
		 */
		if (!addr) {
			skipped++;
			continue;
		}

		end_offset = (pg->index+1) * sizeof(pg->records[0]);
		if (end_offset > PAGE_SIZE << pg->order) {
			/* We should have allocated enough */
			if (WARN_ON(!pg->next))
				break;
			pg = pg->next;
		}

		rec = &pg->records[pg->index++];
		rec->ip = addr;
	}

	if (pg->next) {
		pg_unuse = pg->next;
		pg->next = NULL;
	}

	/* Assign the last page to ftrace_pages */
	ftrace_pages = pg;

	/*
	 * We only need to disable interrupts on start up
	 * because we are modifying code that an interrupt
	 * may execute, and the modification is not atomic.
	 * But for modules, nothing runs the code we modify
	 * until we are finished with it, and there's no
	 * reason to cause large interrupt latencies while we do it.
	 */
	if (!mod)
		local_irq_save(flags);
	ftrace_update_code(mod, start_pg);
	if (!mod)
		local_irq_restore(flags);
	ret = 0;
 out:
	mutex_unlock(&ftrace_lock);

	/* We should have used all pages unless we skipped some */
	if (pg_unuse) {
		WARN_ON(!skipped);
		/* Need to synchronize with ftrace_location_range() */
		synchronize_rcu();
		ftrace_free_pages(pg_unuse);
	}
	return ret;
}
