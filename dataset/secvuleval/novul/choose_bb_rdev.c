static int choose_bb_rdev(struct r1conf *conf, struct r1bio *r1_bio,
			  int *max_sectors)
{
	sector_t this_sector = r1_bio->sector;
	int best_disk = -1;
	int best_len = 0;
	int disk;

	for (disk = 0 ; disk < conf->raid_disks * 2 ; disk++) {
		struct md_rdev *rdev;
		int len;
		int read_len;

		if (r1_bio->bios[disk] == IO_BLOCKED)
			continue;

		rdev = conf->mirrors[disk].rdev;
		if (!rdev || test_bit(Faulty, &rdev->flags) ||
		    rdev_in_recovery(rdev, r1_bio) ||
		    test_bit(WriteMostly, &rdev->flags))
			continue;

		/* keep track of the disk with the most readable sectors. */
		len = r1_bio->sectors;
		read_len = raid1_check_read_range(rdev, this_sector, &len);
		if (read_len > best_len) {
			best_disk = disk;
			best_len = read_len;
		}
	}

	if (best_disk != -1) {
		*max_sectors = best_len;
		update_read_sectors(conf, best_disk, this_sector, best_len);
	}

	return best_disk;
}
