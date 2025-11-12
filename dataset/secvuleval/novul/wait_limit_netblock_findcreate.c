wait_limit_netblock_findcreate(struct infra_cache* infra, char* str,
	int cookie)
{
	rbtree_type* tree;
	struct sockaddr_storage addr;
	int net;
	socklen_t addrlen;
	struct wait_limit_netblock_info* d;

	if(!netblockstrtoaddr(str, 0, &addr, &addrlen, &net)) {
		log_err("cannot parse wait limit netblock '%s'", str);
		return 0;
	}

	/* can we find it? */
	if(cookie)
		tree = &infra->wait_limits_cookie_netblock;
	else
		tree = &infra->wait_limits_netblock;
	d = (struct wait_limit_netblock_info*)addr_tree_find(tree, &addr,
		addrlen, net);
	if(d)
		return d;

	/* create it */
	d = (struct wait_limit_netblock_info*)calloc(1, sizeof(*d));
	if(!d)
		return NULL;
	d->limit = -1;
	if(!addr_tree_insert(tree, &d->node, &addr, addrlen, net)) {
		log_err("duplicate element in domainlimit tree");
		free(d);
		return NULL;
	}
	return d;
}