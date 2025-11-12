void infra_wait_limit_dec(struct infra_cache* infra, struct comm_reply* rep,
	struct config_file* cfg)
{
	struct lruhash_entry* entry;
	if(cfg->wait_limit == 0)
		return;

	entry = infra_find_ip_ratedata(infra, &rep->client_addr,
		rep->client_addrlen, 1);
	if(entry) {
		struct rate_data* d = (struct rate_data*)entry->data;
		if(d->mesh_wait > 0)
			d->mesh_wait--;
		lock_rw_unlock(&entry->lock);
	}
}