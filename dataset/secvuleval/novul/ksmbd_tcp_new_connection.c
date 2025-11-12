static int ksmbd_tcp_new_connection(struct socket *client_sk)
{
	struct sockaddr *csin;
	int rc = 0;
	struct tcp_transport *t;
	struct task_struct *handler;

	t = alloc_transport(client_sk);
	if (!t) {
		sock_release(client_sk);
		return -ENOMEM;
	}

	csin = KSMBD_TCP_PEER_SOCKADDR(KSMBD_TRANS(t)->conn);
	if (kernel_getpeername(client_sk, csin) < 0) {
		pr_err("client ip resolution failed\n");
		rc = -EINVAL;
		goto out_error;
	}

	handler = kthread_run(ksmbd_conn_handler_loop,
			      KSMBD_TRANS(t)->conn,
			      "ksmbd:%u",
			      ksmbd_tcp_get_port(csin));
	if (IS_ERR(handler)) {
		pr_err("cannot start conn thread\n");
		rc = PTR_ERR(handler);
		free_transport(t);
	}
	return rc;

out_error:
	free_transport(t);
	return rc;
}
