static struct tracing_map_elt *get_free_elt(struct tracing_map *map)
{
	struct tracing_map_elt *elt = NULL;
	int idx;

	idx = atomic_fetch_add_unless(&map->next_elt, 1, map->max_elts);
	if (idx < map->max_elts) {
		elt = *(TRACING_MAP_ELT(map->elts, idx));
		if (map->ops && map->ops->elt_init)
			map->ops->elt_init(elt);
	}

	return elt;
}
