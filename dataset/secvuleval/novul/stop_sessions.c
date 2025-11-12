static void stop_sessions(void)
{
	struct ksmbd_conn *conn;
	struct ksmbd_transport *t;

again:
	down_read(&conn_list_lock);
	list_for_each_entry(conn, &conn_list, conns_list) {
		t = conn->transport;
		ksmbd_conn_set_exiting(conn);
		if (t->ops->shutdown) {
			up_read(&conn_list_lock);
			t->ops->shutdown(t);
			down_read(&conn_list_lock);
		}
	}
	up_read(&conn_list_lock);

	if (!list_empty(&conn_list)) {
		schedule_timeout_interruptible(HZ / 10); /* 100ms */
		goto again;
	}
}
