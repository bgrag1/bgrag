static int btrfs_load_zone_info(struct btrfs_fs_info *fs_info, int zone_idx,
				struct zone_info *info, unsigned long *active,
				struct btrfs_chunk_map *map)
{
	struct btrfs_dev_replace *dev_replace = &fs_info->dev_replace;
	struct btrfs_device *device = map->stripes[zone_idx].dev;
	int dev_replace_is_ongoing = 0;
	unsigned int nofs_flag;
	struct blk_zone zone;
	int ret;

	info->physical = map->stripes[zone_idx].physical;

	if (!device->bdev) {
		info->alloc_offset = WP_MISSING_DEV;
		return 0;
	}

	/* Consider a zone as active if we can allow any number of active zones. */
	if (!device->zone_info->max_active_zones)
		__set_bit(zone_idx, active);

	if (!btrfs_dev_is_sequential(device, info->physical)) {
		info->alloc_offset = WP_CONVENTIONAL;
		return 0;
	}

	/* This zone will be used for allocation, so mark this zone non-empty. */
	btrfs_dev_clear_zone_empty(device, info->physical);

	down_read(&dev_replace->rwsem);
	dev_replace_is_ongoing = btrfs_dev_replace_is_ongoing(dev_replace);
	if (dev_replace_is_ongoing && dev_replace->tgtdev != NULL)
		btrfs_dev_clear_zone_empty(dev_replace->tgtdev, info->physical);
	up_read(&dev_replace->rwsem);

	/*
	 * The group is mapped to a sequential zone. Get the zone write pointer
	 * to determine the allocation offset within the zone.
	 */
	WARN_ON(!IS_ALIGNED(info->physical, fs_info->zone_size));
	nofs_flag = memalloc_nofs_save();
	ret = btrfs_get_dev_zone(device, info->physical, &zone);
	memalloc_nofs_restore(nofs_flag);
	if (ret) {
		if (ret != -EIO && ret != -EOPNOTSUPP)
			return ret;
		info->alloc_offset = WP_MISSING_DEV;
		return 0;
	}

	if (zone.type == BLK_ZONE_TYPE_CONVENTIONAL) {
		btrfs_err_in_rcu(fs_info,
		"zoned: unexpected conventional zone %llu on device %s (devid %llu)",
			zone.start << SECTOR_SHIFT, rcu_str_deref(device->name),
			device->devid);
		return -EIO;
	}

	info->capacity = (zone.capacity << SECTOR_SHIFT);

	switch (zone.cond) {
	case BLK_ZONE_COND_OFFLINE:
	case BLK_ZONE_COND_READONLY:
		btrfs_err(fs_info,
		"zoned: offline/readonly zone %llu on device %s (devid %llu)",
			  (info->physical >> device->zone_info->zone_size_shift),
			  rcu_str_deref(device->name), device->devid);
		info->alloc_offset = WP_MISSING_DEV;
		break;
	case BLK_ZONE_COND_EMPTY:
		info->alloc_offset = 0;
		break;
	case BLK_ZONE_COND_FULL:
		info->alloc_offset = info->capacity;
		break;
	default:
		/* Partially used zone. */
		info->alloc_offset = ((zone.wp - zone.start) << SECTOR_SHIFT);
		__set_bit(zone_idx, active);
		break;
	}

	return 0;
}
