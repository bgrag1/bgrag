static void
list_set_destroy(struct ip_set *set)
{
	struct list_set *map = set->data;
	struct set_elem *e, *n;

	if (SET_WITH_TIMEOUT(set))
		timer_shutdown_sync(&map->gc);

	list_for_each_entry_safe(e, n, &map->members, list) {
		list_del(&e->list);
		ip_set_put_byindex(map->net, e->id);
		ip_set_ext_destroy(set, e);
		kfree(e);
	}
	kfree(map);

	set->data = NULL;
}
