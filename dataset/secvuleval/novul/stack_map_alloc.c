static struct bpf_map *stack_map_alloc(union bpf_attr *attr)
{
	u32 value_size = attr->value_size;
	struct bpf_stack_map *smap;
	u64 cost, n_buckets;
	int err;

	if (attr->map_flags & ~STACK_CREATE_FLAG_MASK)
		return ERR_PTR(-EINVAL);

	/* check sanity of attributes */
	if (attr->max_entries == 0 || attr->key_size != 4 ||
	    value_size < 8 || value_size % 8)
		return ERR_PTR(-EINVAL);

	BUILD_BUG_ON(sizeof(struct bpf_stack_build_id) % sizeof(u64));
	if (attr->map_flags & BPF_F_STACK_BUILD_ID) {
		if (value_size % sizeof(struct bpf_stack_build_id) ||
		    value_size / sizeof(struct bpf_stack_build_id)
		    > sysctl_perf_event_max_stack)
			return ERR_PTR(-EINVAL);
	} else if (value_size / 8 > sysctl_perf_event_max_stack)
		return ERR_PTR(-EINVAL);

	/* hash table size must be power of 2; roundup_pow_of_two() can overflow
	 * into UB on 32-bit arches, so check that first
	 */
	if (attr->max_entries > 1UL << 31)
		return ERR_PTR(-E2BIG);

	n_buckets = roundup_pow_of_two(attr->max_entries);

	cost = n_buckets * sizeof(struct stack_map_bucket *) + sizeof(*smap);
	smap = bpf_map_area_alloc(cost, bpf_map_attr_numa_node(attr));
	if (!smap)
		return ERR_PTR(-ENOMEM);

	bpf_map_init_from_attr(&smap->map, attr);
	smap->n_buckets = n_buckets;

	err = get_callchain_buffers(sysctl_perf_event_max_stack);
	if (err)
		goto free_smap;

	err = prealloc_elems_and_freelist(smap);
	if (err)
		goto put_buffers;

	return &smap->map;

put_buffers:
	put_callchain_buffers();
free_smap:
	bpf_map_area_free(smap);
	return ERR_PTR(err);
}
