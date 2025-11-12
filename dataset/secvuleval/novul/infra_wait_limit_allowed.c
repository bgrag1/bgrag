int infra_wait_limit_allowed(struct infra_cache* infra, struct comm_reply* rep,
	int cookie_valid, struct config_file* cfg)
{
	struct lruhash_entry* entry;
	if(cfg->wait_limit == 0)
		return 1;

	entry = infra_find_ip_ratedata(infra, &rep->client_addr,
		rep->client_addrlen, 0);
	if(entry) {
		rbtree_type* tree;
		struct wait_limit_netblock_info* w;
		struct rate_data* d = (struct rate_data*)entry->data;
		int mesh_wait = d->mesh_wait;
		lock_rw_unlock(&entry->lock);

		/* have the wait amount, check how much is allowed */
		if(cookie_valid)
			tree = &infra->wait_limits_cookie_netblock;
		else	tree = &infra->wait_limits_netblock;
		w = (struct wait_limit_netblock_info*)addr_tree_lookup(tree,
			&rep->client_addr, rep->client_addrlen);
		if(w) {
			if(w->limit != -1 && mesh_wait > w->limit)
				return 0;
		} else {
			/* if there is no IP netblock specific information,
			 * use the configured value. */
			if(mesh_wait > (cookie_valid?cfg->wait_limit_cookie:
				cfg->wait_limit))
				return 0;
		}
	}
	return 1;
}