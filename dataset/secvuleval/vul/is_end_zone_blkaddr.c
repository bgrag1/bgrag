static bool is_end_zone_blkaddr(struct f2fs_sb_info *sbi, block_t blkaddr)
{
	int devi = 0;

	if (f2fs_is_multi_device(sbi)) {
		devi = f2fs_target_device_index(sbi, blkaddr);
		if (blkaddr < FDEV(devi).start_blk ||
		    blkaddr > FDEV(devi).end_blk) {
			f2fs_err(sbi, "Invalid block %x", blkaddr);
			return false;
		}
		blkaddr -= FDEV(devi).start_blk;
	}
	return bdev_is_zoned(FDEV(devi).bdev) &&
		f2fs_blkz_is_seq(sbi, devi, blkaddr) &&
		(blkaddr % sbi->blocks_per_blkz == sbi->blocks_per_blkz - 1);
}
