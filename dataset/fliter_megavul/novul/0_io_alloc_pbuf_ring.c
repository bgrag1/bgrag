static int io_alloc_pbuf_ring(struct io_ring_ctx *ctx,
			      struct io_uring_buf_reg *reg,
			      struct io_buffer_list *bl)
{
	struct io_buf_free *ibf;
	size_t ring_size;
	void *ptr;

	ring_size = reg->ring_entries * sizeof(struct io_uring_buf_ring);
	ptr = io_mem_alloc(ring_size);
	if (!ptr)
		return -ENOMEM;

	/* Allocate and store deferred free entry */
	ibf = kmalloc(sizeof(*ibf), GFP_KERNEL_ACCOUNT);
	if (!ibf) {
		io_mem_free(ptr);
		return -ENOMEM;
	}
	ibf->mem = ptr;
	hlist_add_head(&ibf->list, &ctx->io_buf_list);

	bl->buf_ring = ptr;
	bl->is_mapped = 1;
	bl->is_mmap = 1;
	return 0;
}