void cachefiles_withdraw_cache(struct cachefiles_cache *cache)
{
	struct fscache_cache *fscache = cache->cache;

	pr_info("File cache on %s unregistering\n", fscache->name);

	fscache_withdraw_cache(fscache);
	cachefiles_withdraw_fscache_volumes(cache);

	/* we now have to destroy all the active objects pertaining to this
	 * cache - which we do by passing them off to thread pool to be
	 * disposed of */
	cachefiles_withdraw_objects(cache);
	fscache_wait_for_objects(fscache);

	cachefiles_withdraw_volumes(cache);
	cachefiles_sync_cache(cache);
	cache->cache = NULL;
	fscache_relinquish_cache(fscache);
}
