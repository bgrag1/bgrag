static struct mptcp_pm_addr_entry *
select_signal_address(struct pm_nl_pernet *pernet, const struct mptcp_sock *msk)
{
	struct mptcp_pm_addr_entry *entry, *ret = NULL;

	rcu_read_lock();
	/* do not keep any additional per socket state, just signal
	 * the address list in order.
	 * Note: removal from the local address list during the msk life-cycle
	 * can lead to additional addresses not being announced.
	 */
	list_for_each_entry_rcu(entry, &pernet->local_addr_list, list) {
		if (!test_bit(entry->addr.id, msk->pm.id_avail_bitmap))
			continue;

		if (!(entry->flags & MPTCP_PM_ADDR_FLAG_SIGNAL))
			continue;

		ret = entry;
		break;
	}
	rcu_read_unlock();
	return ret;
}
