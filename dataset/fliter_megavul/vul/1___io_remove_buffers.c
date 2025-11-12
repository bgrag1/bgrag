static int __io_remove_buffers(struct io_ring_ctx *ctx,
			       struct io_buffer_list *bl, unsigned nbufs)
{
	unsigned i = 0;

	/* shouldn't happen */
	if (!nbufs)
		return 0;

	if (bl->is_mapped) {
		i = bl->buf_ring->tail - bl->head;
		if (bl->is_mmap) {
			folio_put(virt_to_folio(bl->buf_ring));
			bl->buf_ring = NULL;
			bl->is_mmap = 0;
		} else if (bl->buf_nr_pages) {
			int j;

			for (j = 0; j < bl->buf_nr_pages; j++)
				unpin_user_page(bl->buf_pages[j]);
			kvfree(bl->buf_pages);
			bl->buf_pages = NULL;
			bl->buf_nr_pages = 0;
		}
		/* make sure it's seen as empty */
		INIT_LIST_HEAD(&bl->buf_list);
		bl->is_mapped = 0;
		return i;
	}

	/* protects io_buffers_cache */
	lockdep_assert_held(&ctx->uring_lock);

	while (!list_empty(&bl->buf_list)) {
		struct io_buffer *nxt;

		nxt = list_first_entry(&bl->buf_list, struct io_buffer, list);
		list_move(&nxt->list, &ctx->io_buffers_cache);
		if (++i == nbufs)
			return i;
		cond_resched();
	}

	return i;
}