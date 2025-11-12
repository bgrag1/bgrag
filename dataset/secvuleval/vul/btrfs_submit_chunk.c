static bool btrfs_submit_chunk(struct btrfs_bio *bbio, int mirror_num)
{
	struct btrfs_inode *inode = bbio->inode;
	struct btrfs_fs_info *fs_info = bbio->fs_info;
	struct btrfs_bio *orig_bbio = bbio;
	struct bio *bio = &bbio->bio;
	u64 logical = bio->bi_iter.bi_sector << SECTOR_SHIFT;
	u64 length = bio->bi_iter.bi_size;
	u64 map_length = length;
	bool use_append = btrfs_use_zone_append(bbio);
	struct btrfs_io_context *bioc = NULL;
	struct btrfs_io_stripe smap;
	blk_status_t ret;
	int error;

	smap.is_scrub = !bbio->inode;

	btrfs_bio_counter_inc_blocked(fs_info);
	error = btrfs_map_block(fs_info, btrfs_op(bio), logical, &map_length,
				&bioc, &smap, &mirror_num);
	if (error) {
		ret = errno_to_blk_status(error);
		goto fail;
	}

	map_length = min(map_length, length);
	if (use_append)
		map_length = min(map_length, fs_info->max_zone_append_size);

	if (map_length < length) {
		bbio = btrfs_split_bio(fs_info, bbio, map_length, use_append);
		bio = &bbio->bio;
	}

	/*
	 * Save the iter for the end_io handler and preload the checksums for
	 * data reads.
	 */
	if (bio_op(bio) == REQ_OP_READ && is_data_bbio(bbio)) {
		bbio->saved_iter = bio->bi_iter;
		ret = btrfs_lookup_bio_sums(bbio);
		if (ret)
			goto fail_put_bio;
	}

	if (btrfs_op(bio) == BTRFS_MAP_WRITE) {
		if (use_append) {
			bio->bi_opf &= ~REQ_OP_WRITE;
			bio->bi_opf |= REQ_OP_ZONE_APPEND;
		}

		if (is_data_bbio(bbio) && bioc &&
		    btrfs_need_stripe_tree_update(bioc->fs_info, bioc->map_type)) {
			/*
			 * No locking for the list update, as we only add to
			 * the list in the I/O submission path, and list
			 * iteration only happens in the completion path, which
			 * can't happen until after the last submission.
			 */
			btrfs_get_bioc(bioc);
			list_add_tail(&bioc->rst_ordered_entry, &bbio->ordered->bioc_list);
		}

		/*
		 * Csum items for reloc roots have already been cloned at this
		 * point, so they are handled as part of the no-checksum case.
		 */
		if (inode && !(inode->flags & BTRFS_INODE_NODATASUM) &&
		    !test_bit(BTRFS_FS_STATE_NO_DATA_CSUMS, &fs_info->fs_state) &&
		    !btrfs_is_data_reloc_root(inode->root)) {
			if (should_async_write(bbio) &&
			    btrfs_wq_submit_bio(bbio, bioc, &smap, mirror_num))
				goto done;

			ret = btrfs_bio_csum(bbio);
			if (ret)
				goto fail_put_bio;
		} else if (use_append ||
			   (btrfs_is_zoned(fs_info) && inode &&
			    inode->flags & BTRFS_INODE_NODATASUM)) {
			ret = btrfs_alloc_dummy_sum(bbio);
			if (ret)
				goto fail_put_bio;
		}
	}

	__btrfs_submit_bio(bio, bioc, &smap, mirror_num);
done:
	return map_length == length;

fail_put_bio:
	if (map_length < length)
		btrfs_cleanup_bio(bbio);
fail:
	btrfs_bio_counter_dec(fs_info);
	btrfs_bio_end_io(orig_bbio, ret);
	/* Do not submit another chunk */
	return true;
}
