static void dmirror_device_evict_chunk(struct dmirror_chunk *chunk)
{
	unsigned long start_pfn = chunk->pagemap.range.start >> PAGE_SHIFT;
	unsigned long end_pfn = chunk->pagemap.range.end >> PAGE_SHIFT;
	unsigned long npages = end_pfn - start_pfn + 1;
	unsigned long i;
	unsigned long *src_pfns;
	unsigned long *dst_pfns;

	src_pfns = kcalloc(npages, sizeof(*src_pfns), GFP_KERNEL);
	dst_pfns = kcalloc(npages, sizeof(*dst_pfns), GFP_KERNEL);

	migrate_device_range(src_pfns, start_pfn, npages);
	for (i = 0; i < npages; i++) {
		struct page *dpage, *spage;

		spage = migrate_pfn_to_page(src_pfns[i]);
		if (!spage || !(src_pfns[i] & MIGRATE_PFN_MIGRATE))
			continue;

		if (WARN_ON(!is_device_private_page(spage) &&
			    !is_device_coherent_page(spage)))
			continue;
		spage = BACKING_PAGE(spage);
		dpage = alloc_page(GFP_HIGHUSER_MOVABLE | __GFP_NOFAIL);
		lock_page(dpage);
		copy_highpage(dpage, spage);
		dst_pfns[i] = migrate_pfn(page_to_pfn(dpage));
		if (src_pfns[i] & MIGRATE_PFN_WRITE)
			dst_pfns[i] |= MIGRATE_PFN_WRITE;
	}
	migrate_device_pages(src_pfns, dst_pfns, npages);
	migrate_device_finalize(src_pfns, dst_pfns, npages);
	kfree(src_pfns);
	kfree(dst_pfns);
}
