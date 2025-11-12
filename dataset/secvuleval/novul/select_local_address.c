static bool
select_local_address(const struct pm_nl_pernet *pernet,
		     const struct mptcp_sock *msk,
		     struct mptcp_pm_addr_entry *new_entry)
{
	struct mptcp_pm_addr_entry *entry;
	bool found = false;

	msk_owned_by_me(msk);

	rcu_read_lock();
	list_for_each_entry_rcu(entry, &pernet->local_addr_list, list) {
		if (!(entry->flags & MPTCP_PM_ADDR_FLAG_SUBFLOW))
			continue;

		if (!test_bit(entry->addr.id, msk->pm.id_avail_bitmap))
			continue;

		*new_entry = *entry;
		found = true;
		break;
	}
	rcu_read_unlock();

	return found;
}
