static int dev_map_init_map(struct bpf_dtab *dtab, union bpf_attr *attr)
{
	u32 valsize = attr->value_size;

	/* check sanity of attributes. 2 value sizes supported:
	 * 4 bytes: ifindex
	 * 8 bytes: ifindex + prog fd
	 */
	if (attr->max_entries == 0 || attr->key_size != 4 ||
	    (valsize != offsetofend(struct bpf_devmap_val, ifindex) &&
	     valsize != offsetofend(struct bpf_devmap_val, bpf_prog.fd)) ||
	    attr->map_flags & ~DEV_CREATE_FLAG_MASK)
		return -EINVAL;

	/* Lookup returns a pointer straight to dev->ifindex, so make sure the
	 * verifier prevents writes from the BPF side
	 */
	attr->map_flags |= BPF_F_RDONLY_PROG;


	bpf_map_init_from_attr(&dtab->map, attr);

	if (attr->map_type == BPF_MAP_TYPE_DEVMAP_HASH) {
		/* hash table size must be power of 2; roundup_pow_of_two() can
		 * overflow into UB on 32-bit arches, so check that first
		 */
		if (dtab->map.max_entries > 1UL << 31)
			return -EINVAL;

		dtab->n_buckets = roundup_pow_of_two(dtab->map.max_entries);

		dtab->dev_index_head = dev_map_create_hash(dtab->n_buckets,
							   dtab->map.numa_node);
		if (!dtab->dev_index_head)
			return -ENOMEM;

		spin_lock_init(&dtab->index_lock);
	} else {
		dtab->netdev_map = bpf_map_area_alloc((u64) dtab->map.max_entries *
						      sizeof(struct bpf_dtab_netdev *),
						      dtab->map.numa_node);
		if (!dtab->netdev_map)
			return -ENOMEM;
	}

	return 0;
}
