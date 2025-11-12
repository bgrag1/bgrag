int infra_ip_ratelimit_inc(struct infra_cache* infra,
	struct sockaddr_storage* addr, socklen_t addrlen, time_t timenow,
	int has_cookie, int backoff, struct sldns_buffer* buffer)
{
	int max;
	struct lruhash_entry* entry;

	/* not enabled */
	if(!infra_ip_ratelimit) {
		return 1;
	}
	/* find or insert ratedata */
	entry = infra_find_ip_ratedata(infra, addr, addrlen, 1);
	if(entry) {
		int premax = infra_rate_max(entry->data, timenow, backoff);
		int* cur = infra_rate_give_second(entry->data, timenow);
		(*cur)++;
		max = infra_rate_max(entry->data, timenow, backoff);
		lock_rw_unlock(&entry->lock);
		return check_ip_ratelimit(addr, addrlen, buffer, premax, max,
			has_cookie);
	}

	/* create */
	infra_ip_create_ratedata(infra, addr, addrlen, timenow);
	return 1;
}