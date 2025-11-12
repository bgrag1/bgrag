infra_delete(struct infra_cache* infra)
{
	if(!infra)
		return;
	slabhash_delete(infra->hosts);
	slabhash_delete(infra->domain_rates);
	traverse_postorder(&infra->domain_limits, domain_limit_free, NULL);
	slabhash_delete(infra->client_ip_rates);
	traverse_postorder(&infra->wait_limits_netblock,
		wait_limit_netblock_del, NULL);
	traverse_postorder(&infra->wait_limits_cookie_netblock,
		wait_limit_netblock_del, NULL);
	free(infra);
}