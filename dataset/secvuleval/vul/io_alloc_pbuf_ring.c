static int io_alloc_pbuf_ring(struct io_ring_ctx *ctx,
			      struct io_uring_buf_reg *reg,
			      struct io_buffer_list *bl)
{
	size_t ring_size;

	ring_size = reg->ring_entries * sizeof(struct io_uring_buf_ring);

	bl->buf_ring = io_pages_map(&bl->buf_pages, &bl->buf_nr_pages, ring_size);
	if (!bl->buf_ring)
		return -ENOMEM;

	bl->is_buf_ring = 1;
	bl->is_mmap = 1;
	return 0;
}
