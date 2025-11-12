static void infra_ip_create_ratedata(struct infra_cache* infra,
	struct sockaddr_storage* addr, socklen_t addrlen, time_t timenow,
	int mesh_wait)
{
	hashvalue_type h = hash_addr(addr, addrlen, 0);
	struct ip_rate_key* k = (struct ip_rate_key*)calloc(1, sizeof(*k));
	struct ip_rate_data* d = (struct ip_rate_data*)calloc(1, sizeof(*d));
	if(!k || !d) {
		free(k);
		free(d);
		return; /* alloc failure */
	}
	k->addr = *addr;
	k->addrlen = addrlen;
	lock_rw_init(&k->entry.lock);
	k->entry.hash = h;
	k->entry.key = k;
	k->entry.data = d;
	d->qps[0] = 1;
	d->timestamp[0] = timenow;
	d->mesh_wait = mesh_wait;
	slabhash_insert(infra->client_ip_rates, h, &k->entry, d, NULL);
}