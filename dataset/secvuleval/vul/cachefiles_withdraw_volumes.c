static void cachefiles_withdraw_volumes(struct cachefiles_cache *cache)
{
	_enter("");

	for (;;) {
		struct cachefiles_volume *volume = NULL;

		spin_lock(&cache->object_list_lock);
		if (!list_empty(&cache->volumes)) {
			volume = list_first_entry(&cache->volumes,
						  struct cachefiles_volume, cache_link);
			list_del_init(&volume->cache_link);
		}
		spin_unlock(&cache->object_list_lock);
		if (!volume)
			break;

		cachefiles_withdraw_volume(volume);
	}

	_leave("");
}
