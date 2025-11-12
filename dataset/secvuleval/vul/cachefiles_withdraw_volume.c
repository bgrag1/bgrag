void cachefiles_withdraw_volume(struct cachefiles_volume *volume)
{
	fscache_withdraw_volume(volume->vcookie);
	cachefiles_set_volume_xattr(volume);
	__cachefiles_free_volume(volume);
}
