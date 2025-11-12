infra_wait_limit_netblock_insert(struct infra_cache* infra,
	struct config_file* cfg)
{
	struct config_str2list* p;
	struct wait_limit_netblock_info* d;
	for(p = cfg->wait_limit_netblock; p; p = p->next) {
		d = wait_limit_netblock_findcreate(infra, p->str, 0);
		if(!d)
			return 0;
		d->limit = atoi(p->str2);
	}
	for(p = cfg->wait_limit_cookie_netblock; p; p = p->next) {
		d = wait_limit_netblock_findcreate(infra, p->str, 1);
		if(!d)
			return 0;
		d->limit = atoi(p->str2);
	}
	return 1;
}