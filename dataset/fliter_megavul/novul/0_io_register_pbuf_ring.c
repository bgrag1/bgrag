int io_register_pbuf_ring(struct io_ring_ctx *ctx, void __user *arg)
{
	struct io_uring_buf_reg reg;
	struct io_buffer_list *bl, *free_bl = NULL;
	int ret;

	if (copy_from_user(&reg, arg, sizeof(reg)))
		return -EFAULT;

	if (reg.resv[0] || reg.resv[1] || reg.resv[2])
		return -EINVAL;
	if (reg.flags & ~IOU_PBUF_RING_MMAP)
		return -EINVAL;
	if (!(reg.flags & IOU_PBUF_RING_MMAP)) {
		if (!reg.ring_addr)
			return -EFAULT;
		if (reg.ring_addr & ~PAGE_MASK)
			return -EINVAL;
	} else {
		if (reg.ring_addr)
			return -EINVAL;
	}

	if (!is_power_of_2(reg.ring_entries))
		return -EINVAL;

	/* cannot disambiguate full vs empty due to head/tail size */
	if (reg.ring_entries >= 65536)
		return -EINVAL;

	if (unlikely(reg.bgid < BGID_ARRAY && !ctx->io_bl)) {
		int ret = io_init_bl_list(ctx);
		if (ret)
			return ret;
	}

	bl = io_buffer_get_list(ctx, reg.bgid);
	if (bl) {
		/* if mapped buffer ring OR classic exists, don't allow */
		if (bl->is_mapped || !list_empty(&bl->buf_list))
			return -EEXIST;
	} else {
		free_bl = bl = kzalloc(sizeof(*bl), GFP_KERNEL);
		if (!bl)
			return -ENOMEM;
	}

	if (!(reg.flags & IOU_PBUF_RING_MMAP))
		ret = io_pin_pbuf_ring(&reg, bl);
	else
		ret = io_alloc_pbuf_ring(ctx, &reg, bl);

	if (!ret) {
		bl->nr_entries = reg.ring_entries;
		bl->mask = reg.ring_entries - 1;

		io_buffer_add_list(ctx, bl, reg.bgid);
		return 0;
	}

	kfree(free_bl);
	return ret;
}