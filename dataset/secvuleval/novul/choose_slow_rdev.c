static int choose_slow_rdev(struct r1conf *conf, struct r1bio *r1_bio,
			    int *max_sectors)
{
	sector_t this_sector = r1_bio->sector;
	int bb_disk = -1;
	int bb_read_len = 0;
	int disk;

	for (disk = 0 ; disk < conf->raid_disks * 2 ; disk++) {
		struct md_rdev *rdev;
		int len;
		int read_len;

		if (r1_bio->bios[disk] == IO_BLOCKED)
			continue;

		rdev = conf->mirrors[disk].rdev;
		if (!rdev || test_bit(Faulty, &rdev->flags) ||
		    !test_bit(WriteMostly, &rdev->flags) ||
		    rdev_in_recovery(rdev, r1_bio))
			continue;

		/* there are no bad blocks, we can use this disk */
		len = r1_bio->sectors;
		read_len = raid1_check_read_range(rdev, this_sector, &len);
		if (read_len == r1_bio->sectors) {
			*max_sectors = read_len;
			update_read_sectors(conf, disk, this_sector, read_len);
			return disk;
		}

		/*
		 * there are partial bad blocks, choose the rdev with largest
		 * read length.
		 */
		if (read_len > bb_read_len) {
			bb_disk = disk;
			bb_read_len = read_len;
		}
	}

	if (bb_disk != -1) {
		*max_sectors = bb_read_len;
		update_read_sectors(conf, bb_disk, this_sector, bb_read_len);
	}

	return bb_disk;
}
