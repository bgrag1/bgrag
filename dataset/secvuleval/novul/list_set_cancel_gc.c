static void
list_set_cancel_gc(struct ip_set *set)
{
	struct list_set *map = set->data;

	if (SET_WITH_TIMEOUT(set))
		timer_shutdown_sync(&map->gc);
}
