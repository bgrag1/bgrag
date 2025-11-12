static int io_alloc_pbuf_ring(struct io_uring_buf_reg *reg,
			      struct io_buffer_list *bl)
{
	gfp_t gfp = GFP_KERNEL_ACCOUNT | __GFP_ZERO | __GFP_NOWARN | __GFP_COMP;
	size_t ring_size;
	void *ptr;

	ring_size = reg->ring_entries * sizeof(struct io_uring_buf_ring);
	ptr = (void *) __get_free_pages(gfp, get_order(ring_size));
	if (!ptr)
		return -ENOMEM;

	bl->buf_ring = ptr;
	bl->is_mapped = 1;
	bl->is_mmap = 1;
	return 0;
}