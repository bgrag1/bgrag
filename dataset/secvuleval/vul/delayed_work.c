static void delayed_work(struct work_struct *work)
{
	struct ceph_mon_client *monc =
		container_of(work, struct ceph_mon_client, delayed_work.work);

	dout("monc delayed_work\n");
	mutex_lock(&monc->mutex);
	if (monc->hunting) {
		dout("%s continuing hunt\n", __func__);
		reopen_session(monc);
	} else {
		int is_auth = ceph_auth_is_authenticated(monc->auth);
		if (ceph_con_keepalive_expired(&monc->con,
					       CEPH_MONC_PING_TIMEOUT)) {
			dout("monc keepalive timeout\n");
			is_auth = 0;
			reopen_session(monc);
		}

		if (!monc->hunting) {
			ceph_con_keepalive(&monc->con);
			__validate_auth(monc);
			un_backoff(monc);
		}

		if (is_auth &&
		    !(monc->con.peer_features & CEPH_FEATURE_MON_STATEFUL_SUB)) {
			unsigned long now = jiffies;

			dout("%s renew subs? now %lu renew after %lu\n",
			     __func__, now, monc->sub_renew_after);
			if (time_after_eq(now, monc->sub_renew_after))
				__send_subscribe(monc);
		}
	}
	__schedule_delayed(monc);
	mutex_unlock(&monc->mutex);
}
