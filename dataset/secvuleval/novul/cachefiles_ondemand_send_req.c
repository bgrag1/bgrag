static int cachefiles_ondemand_send_req(struct cachefiles_object *object,
					enum cachefiles_opcode opcode,
					size_t data_len,
					init_req_fn init_req,
					void *private)
{
	struct cachefiles_cache *cache = object->volume->cache;
	struct cachefiles_req *req = NULL;
	XA_STATE(xas, &cache->reqs, 0);
	int ret;

	if (!test_bit(CACHEFILES_ONDEMAND_MODE, &cache->flags))
		return 0;

	if (test_bit(CACHEFILES_DEAD, &cache->flags)) {
		ret = -EIO;
		goto out;
	}

	req = kzalloc(sizeof(*req) + data_len, GFP_KERNEL);
	if (!req) {
		ret = -ENOMEM;
		goto out;
	}

	refcount_set(&req->ref, 1);
	req->object = object;
	init_completion(&req->done);
	req->msg.opcode = opcode;
	req->msg.len = sizeof(struct cachefiles_msg) + data_len;

	ret = init_req(req, private);
	if (ret)
		goto out;

	do {
		/*
		 * Stop enqueuing the request when daemon is dying. The
		 * following two operations need to be atomic as a whole.
		 *   1) check cache state, and
		 *   2) enqueue request if cache is alive.
		 * Otherwise the request may be enqueued after xarray has been
		 * flushed, leaving the orphan request never being completed.
		 *
		 * CPU 1			CPU 2
		 * =====			=====
		 *				test CACHEFILES_DEAD bit
		 * set CACHEFILES_DEAD bit
		 * flush requests in the xarray
		 *				enqueue the request
		 */
		xas_lock(&xas);

		if (test_bit(CACHEFILES_DEAD, &cache->flags)) {
			xas_unlock(&xas);
			ret = -EIO;
			goto out;
		}

		/* coupled with the barrier in cachefiles_flush_reqs() */
		smp_mb();

		if (opcode == CACHEFILES_OP_CLOSE &&
			!cachefiles_ondemand_object_is_open(object)) {
			WARN_ON_ONCE(object->ondemand->ondemand_id == 0);
			xas_unlock(&xas);
			ret = -EIO;
			goto out;
		}

		xas.xa_index = 0;
		xas_find_marked(&xas, UINT_MAX, XA_FREE_MARK);
		if (xas.xa_node == XAS_RESTART)
			xas_set_err(&xas, -EBUSY);
		xas_store(&xas, req);
		xas_clear_mark(&xas, XA_FREE_MARK);
		xas_set_mark(&xas, CACHEFILES_REQ_NEW);
		xas_unlock(&xas);
	} while (xas_nomem(&xas, GFP_KERNEL));

	ret = xas_error(&xas);
	if (ret)
		goto out;

	wake_up_all(&cache->daemon_pollwq);
	wait_for_completion(&req->done);
	ret = req->error;
	cachefiles_req_put(req);
	return ret;
out:
	/* Reset the object to close state in error handling path.
	 * If error occurs after creating the anonymous fd,
	 * cachefiles_ondemand_fd_release() will set object to close.
	 */
	if (opcode == CACHEFILES_OP_OPEN)
		cachefiles_ondemand_set_object_close(object);
	kfree(req);
	return ret;
}
