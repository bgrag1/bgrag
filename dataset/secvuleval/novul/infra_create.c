infra_create(struct config_file* cfg)
{
	struct infra_cache* infra = (struct infra_cache*)calloc(1, 
		sizeof(struct infra_cache));
	size_t maxmem = cfg->infra_cache_numhosts * (sizeof(struct infra_key)+
		sizeof(struct infra_data)+INFRA_BYTES_NAME);
	if(!infra) {
		return NULL;
	}
	infra->hosts = slabhash_create(cfg->infra_cache_slabs,
		INFRA_HOST_STARTSIZE, maxmem, &infra_sizefunc, &infra_compfunc,
		&infra_delkeyfunc, &infra_deldatafunc, NULL);
	if(!infra->hosts) {
		free(infra);
		return NULL;
	}
	infra->host_ttl = cfg->host_ttl;
	infra->infra_keep_probing = cfg->infra_keep_probing;
	infra_dp_ratelimit = cfg->ratelimit;
	infra->domain_rates = slabhash_create(cfg->ratelimit_slabs,
		INFRA_HOST_STARTSIZE, cfg->ratelimit_size,
		&rate_sizefunc, &rate_compfunc, &rate_delkeyfunc,
		&rate_deldatafunc, NULL);
	if(!infra->domain_rates) {
		infra_delete(infra);
		return NULL;
	}
	/* insert config data into ratelimits */
	if(!setup_domain_limits(infra, cfg)) {
		infra_delete(infra);
		return NULL;
	}
	if(!setup_wait_limits(infra, cfg)) {
		infra_delete(infra);
		return NULL;
	}
	infra_ip_ratelimit = cfg->ip_ratelimit;
	infra->client_ip_rates = slabhash_create(cfg->ip_ratelimit_slabs,
	    INFRA_HOST_STARTSIZE, cfg->ip_ratelimit_size, &ip_rate_sizefunc,
	    &ip_rate_compfunc, &ip_rate_delkeyfunc, &ip_rate_deldatafunc, NULL);
	if(!infra->client_ip_rates) {
		infra_delete(infra);
		return NULL;
	}
	return infra;
}