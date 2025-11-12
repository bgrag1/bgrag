static __cold void io_ring_ctx_free(struct io_ring_ctx *ctx)
{
	io_sq_thread_finish(ctx);
	/* __io_rsrc_put_work() may need uring_lock to progress, wait w/o it */
	if (WARN_ON_ONCE(!list_empty(&ctx->rsrc_ref_list)))
		return;

	mutex_lock(&ctx->uring_lock);
	if (ctx->buf_data)
		__io_sqe_buffers_unregister(ctx);
	if (ctx->file_data)
		__io_sqe_files_unregister(ctx);
	io_cqring_overflow_kill(ctx);
	io_eventfd_unregister(ctx);
	io_alloc_cache_free(&ctx->apoll_cache, io_apoll_cache_free);
	io_alloc_cache_free(&ctx->netmsg_cache, io_netmsg_cache_free);
	io_futex_cache_free(ctx);
	io_destroy_buffers(ctx);
	mutex_unlock(&ctx->uring_lock);
	if (ctx->sq_creds)
		put_cred(ctx->sq_creds);
	if (ctx->submitter_task)
		put_task_struct(ctx->submitter_task);

	/* there are no registered resources left, nobody uses it */
	if (ctx->rsrc_node)
		io_rsrc_node_destroy(ctx, ctx->rsrc_node);

	WARN_ON_ONCE(!list_empty(&ctx->rsrc_ref_list));

#if defined(CONFIG_UNIX)
	if (ctx->ring_sock) {
		ctx->ring_sock->file = NULL; /* so that iput() is called */
		sock_release(ctx->ring_sock);
	}
#endif
	WARN_ON_ONCE(!list_empty(&ctx->ltimeout_list));

	io_alloc_cache_free(&ctx->rsrc_node_cache, io_rsrc_node_cache_free);
	if (ctx->mm_account) {
		mmdrop(ctx->mm_account);
		ctx->mm_account = NULL;
	}
	io_rings_free(ctx);
	io_kbuf_mmap_list_free(ctx);

	percpu_ref_exit(&ctx->refs);
	free_uid(ctx->user);
	io_req_caches_free(ctx);
	if (ctx->hash_map)
		io_wq_put_hash(ctx->hash_map);
	kfree(ctx->cancel_table.hbs);
	kfree(ctx->cancel_table_locked.hbs);
	kfree(ctx->io_bl);
	xa_destroy(&ctx->io_bl_xa);
	kfree(ctx);
}