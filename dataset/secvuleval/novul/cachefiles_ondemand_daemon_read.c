ssize_t cachefiles_ondemand_daemon_read(struct cachefiles_cache *cache,
					char __user *_buffer, size_t buflen)
{
	struct cachefiles_req *req;
	struct cachefiles_msg *msg;
	unsigned long id = 0;
	size_t n;
	int ret = 0;
	XA_STATE(xas, &cache->reqs, cache->req_id_next);

	xa_lock(&cache->reqs);
	/*
	 * Cyclically search for a request that has not ever been processed,
	 * to prevent requests from being processed repeatedly, and make
	 * request distribution fair.
	 */
	req = cachefiles_ondemand_select_req(&xas, ULONG_MAX);
	if (!req && cache->req_id_next > 0) {
		xas_set(&xas, 0);
		req = cachefiles_ondemand_select_req(&xas, cache->req_id_next - 1);
	}
	if (!req) {
		xa_unlock(&cache->reqs);
		return 0;
	}

	msg = &req->msg;
	n = msg->len;

	if (n > buflen) {
		xa_unlock(&cache->reqs);
		return -EMSGSIZE;
	}

	xas_clear_mark(&xas, CACHEFILES_REQ_NEW);
	cache->req_id_next = xas.xa_index + 1;
	refcount_inc(&req->ref);
	cachefiles_grab_object(req->object, cachefiles_obj_get_read_req);
	xa_unlock(&cache->reqs);

	id = xas.xa_index;

	if (msg->opcode == CACHEFILES_OP_OPEN) {
		ret = cachefiles_ondemand_get_fd(req);
		if (ret) {
			cachefiles_ondemand_set_object_close(req->object);
			goto error;
		}
	}

	msg->msg_id = id;
	msg->object_id = req->object->ondemand->ondemand_id;

	if (copy_to_user(_buffer, msg, n) != 0) {
		ret = -EFAULT;
		goto err_put_fd;
	}

	cachefiles_put_object(req->object, cachefiles_obj_put_read_req);
	/* CLOSE request has no reply */
	if (msg->opcode == CACHEFILES_OP_CLOSE) {
		xa_erase(&cache->reqs, id);
		complete(&req->done);
	}

	cachefiles_req_put(req);
	return n;

err_put_fd:
	if (msg->opcode == CACHEFILES_OP_OPEN)
		close_fd(((struct cachefiles_open *)msg->data)->fd);
error:
	cachefiles_put_object(req->object, cachefiles_obj_put_read_req);
	xas_reset(&xas);
	xas_lock(&xas);
	if (xas_load(&xas) == req) {
		req->error = ret;
		complete(&req->done);
		xas_store(&xas, NULL);
	}
	xas_unlock(&xas);
	cachefiles_req_put(req);
	return ret;
}
