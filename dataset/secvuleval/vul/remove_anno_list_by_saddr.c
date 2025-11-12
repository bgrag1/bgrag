static bool remove_anno_list_by_saddr(struct mptcp_sock *msk,
				      const struct mptcp_addr_info *addr)
{
	struct mptcp_pm_add_entry *entry;

	entry = mptcp_pm_del_add_timer(msk, addr, false);
	if (entry) {
		list_del(&entry->list);
		kfree(entry);
		return true;
	}

	return false;
}
