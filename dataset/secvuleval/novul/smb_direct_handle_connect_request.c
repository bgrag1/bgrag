static int smb_direct_handle_connect_request(struct rdma_cm_id *new_cm_id)
{
	struct smb_direct_transport *t;
	struct task_struct *handler;
	int ret;

	if (!rdma_frwr_is_supported(&new_cm_id->device->attrs)) {
		ksmbd_debug(RDMA,
			    "Fast Registration Work Requests is not supported. device capabilities=%llx\n",
			    new_cm_id->device->attrs.device_cap_flags);
		return -EPROTONOSUPPORT;
	}

	t = alloc_transport(new_cm_id);
	if (!t)
		return -ENOMEM;

	ret = smb_direct_connect(t);
	if (ret)
		goto out_err;

	handler = kthread_run(ksmbd_conn_handler_loop,
			      KSMBD_TRANS(t)->conn, "ksmbd:r%u",
			      smb_direct_port);
	if (IS_ERR(handler)) {
		ret = PTR_ERR(handler);
		pr_err("Can't start thread\n");
		goto out_err;
	}

	return 0;
out_err:
	free_transport(t);
	return ret;
}
