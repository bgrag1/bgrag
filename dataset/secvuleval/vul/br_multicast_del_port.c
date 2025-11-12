void br_multicast_del_port(struct net_bridge_port *port)
{
	struct net_bridge *br = port->br;
	struct net_bridge_port_group *pg;
	HLIST_HEAD(deleted_head);
	struct hlist_node *n;

	/* Take care of the remaining groups, only perm ones should be left */
	spin_lock_bh(&br->multicast_lock);
	hlist_for_each_entry_safe(pg, n, &port->mglist, mglist)
		br_multicast_find_del_pg(br, pg);
	hlist_move_list(&br->mcast_gc_list, &deleted_head);
	spin_unlock_bh(&br->multicast_lock);
	br_multicast_gc(&deleted_head);
	br_multicast_port_ctx_deinit(&port->multicast_ctx);
	free_percpu(port->mcast_stats);
}
