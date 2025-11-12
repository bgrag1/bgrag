void infra_wait_limit_inc(struct infra_cache* infra, struct comm_reply* rep,
	time_t timenow, struct config_file* cfg)
{
	struct lruhash_entry* entry;
	if(cfg->wait_limit == 0)
		return;

	/* Find it */
	entry = infra_find_ip_ratedata(infra, &rep->client_addr,
		rep->client_addrlen, 1);
	if(entry) {
		struct rate_data* d = (struct rate_data*)entry->data;
		d->mesh_wait++;
		lock_rw_unlock(&entry->lock);
		return;
	}

	/* Create it */
	infra_ip_create_ratedata(infra, &rep->client_addr,
		rep->client_addrlen, timenow, 1);
}