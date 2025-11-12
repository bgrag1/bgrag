void ceph_monc_stop(struct ceph_mon_client *monc)
{
	dout("stop\n");
	cancel_delayed_work_sync(&monc->delayed_work);

	mutex_lock(&monc->mutex);
	__close_session(monc);
	monc->cur_mon = -1;
	mutex_unlock(&monc->mutex);

	/*
	 * flush msgr queue before we destroy ourselves to ensure that:
	 *  - any work that references our embedded con is finished.
	 *  - any osd_client or other work that may reference an authorizer
	 *    finishes before we shut down the auth subsystem.
	 */
	ceph_msgr_flush();

	ceph_auth_destroy(monc->auth);

	WARN_ON(!RB_EMPTY_ROOT(&monc->generic_request_tree));

	ceph_msg_put(monc->m_auth);
	ceph_msg_put(monc->m_auth_reply);
	ceph_msg_put(monc->m_subscribe);
	ceph_msg_put(monc->m_subscribe_ack);

	kfree(monc->monmap);
}
