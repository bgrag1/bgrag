setup_wait_limits(struct infra_cache* infra, struct config_file* cfg)
{
	addr_tree_init(&infra->wait_limits_netblock);
	addr_tree_init(&infra->wait_limits_cookie_netblock);
	if(!infra_wait_limit_netblock_insert(infra, cfg))
		return 0;
	addr_tree_init_parents(&infra->wait_limits_netblock);
	addr_tree_init_parents(&infra->wait_limits_cookie_netblock);
	return 1;
}