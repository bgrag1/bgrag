io_ring_ctx *io_ring_ctx_alloc(struct io_uring_params *p)
{
	struct io_ring_ctx *ctx;
	int hash_bits;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return NULL;

	xa_init(&ctx->io_bl_xa);

	/*
	 * Use 5 bits less than the max cq entries, that should give us around
	 * 32 entries per hash list if totally full and uniformly spread, but
	 * don't keep too many buckets to not overconsume memory.
	 */
	hash_bits = ilog2(p->cq_entries) - 5;
	hash_bits = clamp(hash_bits, 1, 8);
	if (io_alloc_hash_table(&ctx->cancel_table, hash_bits))
		goto err;
	if (io_alloc_hash_table(&ctx->cancel_table_locked, hash_bits))
		goto err;
	if (percpu_ref_init(&ctx->refs, io_ring_ctx_ref_free,
			    0, GFP_KERNEL))
		goto err;

	ctx->flags = p->flags;
	init_waitqueue_head(&ctx->sqo_sq_wait);
	INIT_LIST_HEAD(&ctx->sqd_list);
	INIT_LIST_HEAD(&ctx->cq_overflow_list);
	INIT_LIST_HEAD(&ctx->io_buffers_cache);
	io_alloc_cache_init(&ctx->rsrc_node_cache, IO_NODE_ALLOC_CACHE_MAX,
			    sizeof(struct io_rsrc_node));
	io_alloc_cache_init(&ctx->apoll_cache, IO_ALLOC_CACHE_MAX,
			    sizeof(struct async_poll));
	io_alloc_cache_init(&ctx->netmsg_cache, IO_ALLOC_CACHE_MAX,
			    sizeof(struct io_async_msghdr));
	io_futex_cache_init(ctx);
	init_completion(&ctx->ref_comp);
	xa_init_flags(&ctx->personalities, XA_FLAGS_ALLOC1);
	mutex_init(&ctx->uring_lock);
	init_waitqueue_head(&ctx->cq_wait);
	init_waitqueue_head(&ctx->poll_wq);
	init_waitqueue_head(&ctx->rsrc_quiesce_wq);
	spin_lock_init(&ctx->completion_lock);
	spin_lock_init(&ctx->timeout_lock);
	INIT_WQ_LIST(&ctx->iopoll_list);
	INIT_LIST_HEAD(&ctx->io_buffers_comp);
	INIT_LIST_HEAD(&ctx->defer_list);
	INIT_LIST_HEAD(&ctx->timeout_list);
	INIT_LIST_HEAD(&ctx->ltimeout_list);
	INIT_LIST_HEAD(&ctx->rsrc_ref_list);
	init_llist_head(&ctx->work_llist);
	INIT_LIST_HEAD(&ctx->tctx_list);
	ctx->submit_state.free_list.next = NULL;
	INIT_WQ_LIST(&ctx->locked_free_list);
	INIT_HLIST_HEAD(&ctx->waitid_list);
#ifdef CONFIG_FUTEX
	INIT_HLIST_HEAD(&ctx->futex_list);
#endif
	INIT_DELAYED_WORK(&ctx->fallback_work, io_fallback_req_func);
	INIT_WQ_LIST(&ctx->submit_state.compl_reqs);
	INIT_HLIST_HEAD(&ctx->cancelable_uring_cmd);
	return ctx;
err:
	kfree(ctx->cancel_table.hbs);
	kfree(ctx->cancel_table_locked.hbs);
	kfree(ctx->io_bl);
	xa_destroy(&ctx->io_bl_xa);
	kfree(ctx);
	return NULL;
}