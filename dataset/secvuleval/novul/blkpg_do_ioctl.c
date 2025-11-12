static int blkpg_do_ioctl(struct block_device *bdev,
			  struct blkpg_partition __user *upart, int op)
{
	struct gendisk *disk = bdev->bd_disk;
	struct blkpg_partition p;
	sector_t start, length, capacity, end;

	if (!capable(CAP_SYS_ADMIN))
		return -EACCES;
	if (copy_from_user(&p, upart, sizeof(struct blkpg_partition)))
		return -EFAULT;
	if (bdev_is_partition(bdev))
		return -EINVAL;

	if (p.pno <= 0)
		return -EINVAL;

	if (op == BLKPG_DEL_PARTITION)
		return bdev_del_partition(disk, p.pno);

	if (p.start < 0 || p.length <= 0 || LLONG_MAX - p.length < p.start)
		return -EINVAL;
	/* Check that the partition is aligned to the block size */
	if (!IS_ALIGNED(p.start | p.length, bdev_logical_block_size(bdev)))
		return -EINVAL;

	start = p.start >> SECTOR_SHIFT;
	length = p.length >> SECTOR_SHIFT;
	capacity = get_capacity(disk);

	if (check_add_overflow(start, length, &end))
		return -EINVAL;

	if (start >= capacity || end > capacity)
		return -EINVAL;

	switch (op) {
	case BLKPG_ADD_PARTITION:
		return bdev_add_partition(disk, p.pno, start, length);
	case BLKPG_RESIZE_PARTITION:
		return bdev_resize_partition(disk, p.pno, start, length);
	default:
		return -EINVAL;
	}
}
