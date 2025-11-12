static inline unsigned int
vm_area_alloc_pages(gfp_t gfp, int nid,
		unsigned int order, unsigned int nr_pages, struct page **pages)
{
	unsigned int nr_allocated = 0;
	gfp_t alloc_gfp = gfp;
	bool nofail = gfp & __GFP_NOFAIL;
	struct page *page;
	int i;

	/*
	 * For order-0 pages we make use of bulk allocator, if
	 * the page array is partly or not at all populated due
	 * to fails, fallback to a single page allocator that is
	 * more permissive.
	 */
	if (!order) {
		/* bulk allocator doesn't support nofail req. officially */
		gfp_t bulk_gfp = gfp & ~__GFP_NOFAIL;

		while (nr_allocated < nr_pages) {
			unsigned int nr, nr_pages_request;

			/*
			 * A maximum allowed request is hard-coded and is 100
			 * pages per call. That is done in order to prevent a
			 * long preemption off scenario in the bulk-allocator
			 * so the range is [1:100].
			 */
			nr_pages_request = min(100U, nr_pages - nr_allocated);

			/* memory allocation should consider mempolicy, we can't
			 * wrongly use nearest node when nid == NUMA_NO_NODE,
			 * otherwise memory may be allocated in only one node,
			 * but mempolicy wants to alloc memory by interleaving.
			 */
			if (IS_ENABLED(CONFIG_NUMA) && nid == NUMA_NO_NODE)
				nr = alloc_pages_bulk_array_mempolicy_noprof(bulk_gfp,
							nr_pages_request,
							pages + nr_allocated);

			else
				nr = alloc_pages_bulk_array_node_noprof(bulk_gfp, nid,
							nr_pages_request,
							pages + nr_allocated);

			nr_allocated += nr;
			cond_resched();

			/*
			 * If zero or pages were obtained partly,
			 * fallback to a single page allocator.
			 */
			if (nr != nr_pages_request)
				break;
		}
	} else if (gfp & __GFP_NOFAIL) {
		/*
		 * Higher order nofail allocations are really expensive and
		 * potentially dangerous (pre-mature OOM, disruptive reclaim
		 * and compaction etc.
		 */
		alloc_gfp &= ~__GFP_NOFAIL;
	}

	/* High-order pages or fallback path if "bulk" fails. */
	while (nr_allocated < nr_pages) {
		if (!nofail && fatal_signal_pending(current))
			break;

		if (nid == NUMA_NO_NODE)
			page = alloc_pages_noprof(alloc_gfp, order);
		else
			page = alloc_pages_node_noprof(nid, alloc_gfp, order);
		if (unlikely(!page))
			break;

		/*
		 * Higher order allocations must be able to be treated as
		 * indepdenent small pages by callers (as they can with
		 * small-page vmallocs). Some drivers do their own refcounting
		 * on vmalloc_to_page() pages, some use page->mapping,
		 * page->lru, etc.
		 */
		if (order)
			split_page(page, order);

		/*
		 * Careful, we allocate and map page-order pages, but
		 * tracking is done per PAGE_SIZE page so as to keep the
		 * vm_struct APIs independent of the physical/mapped size.
		 */
		for (i = 0; i < (1U << order); i++)
			pages[nr_allocated + i] = page + i;

		cond_resched();
		nr_allocated += 1U << order;
	}

	return nr_allocated;
}
