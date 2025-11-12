void br_multicast_del_port(struct net_bridge_port *port)
{
	struct net_bridge *br = port->br;
	struct net_bridge_port_group *pg;
	struct hlist_node *n;

	/* Take care of the remaining groups, only perm ones should be left */
	spin_lock_bh(&br->multicast_lock);
	hlist_for_each_entry_safe(pg, n, &port->mglist, mglist)
		br_multicast_find_del_pg(br, pg);
	spin_unlock_bh(&br->multicast_lock);
	flush_work(&br->mcast_gc_work);
	br_multicast_port_ctx_deinit(&port->multicast_ctx);
	free_percpu(port->mcast_stats);
}
