static struct tracing_map_elt *get_free_elt(struct tracing_map *map)
{
	struct tracing_map_elt *elt = NULL;
	int idx;

	idx = atomic_inc_return(&map->next_elt);
	if (idx < map->max_elts) {
		elt = *(TRACING_MAP_ELT(map->elts, idx));
		if (map->ops && map->ops->elt_init)
			map->ops->elt_init(elt);
	}

	return elt;
}
