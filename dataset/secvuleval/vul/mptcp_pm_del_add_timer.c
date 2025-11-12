struct mptcp_pm_add_entry *
mptcp_pm_del_add_timer(struct mptcp_sock *msk,
		       const struct mptcp_addr_info *addr, bool check_id)
{
	struct mptcp_pm_add_entry *entry;
	struct sock *sk = (struct sock *)msk;

	spin_lock_bh(&msk->pm.lock);
	entry = mptcp_lookup_anno_list_by_saddr(msk, addr);
	if (entry && (!check_id || entry->addr.id == addr->id))
		entry->retrans_times = ADD_ADDR_RETRANS_MAX;
	spin_unlock_bh(&msk->pm.lock);

	if (entry && (!check_id || entry->addr.id == addr->id))
		sk_stop_timer_sync(sk, &entry->add_timer);

	return entry;
}
