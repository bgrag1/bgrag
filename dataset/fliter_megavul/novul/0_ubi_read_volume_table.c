int ubi_read_volume_table(struct ubi_device *ubi, struct ubi_attach_info *ai)
{
	int err;
	struct ubi_ainf_volume *av;

	empty_vtbl_record.crc = cpu_to_be32(0xf116c36b);

	/*
	 * The number of supported volumes is limited by the eraseblock size
	 * and by the UBI_MAX_VOLUMES constant.
	 */

	if (ubi->leb_size < UBI_VTBL_RECORD_SIZE) {
		ubi_err(ubi, "LEB size too small for a volume record");
		return -EINVAL;
	}

	ubi->vtbl_slots = ubi->leb_size / UBI_VTBL_RECORD_SIZE;
	if (ubi->vtbl_slots > UBI_MAX_VOLUMES)
		ubi->vtbl_slots = UBI_MAX_VOLUMES;

	ubi->vtbl_size = ubi->vtbl_slots * UBI_VTBL_RECORD_SIZE;
	ubi->vtbl_size = ALIGN(ubi->vtbl_size, ubi->min_io_size);

	av = ubi_find_av(ai, UBI_LAYOUT_VOLUME_ID);
	if (!av) {
		/*
		 * No logical eraseblocks belonging to the layout volume were
		 * found. This could mean that the flash is just empty. In
		 * this case we create empty layout volume.
		 *
		 * But if flash is not empty this must be a corruption or the
		 * MTD device just contains garbage.
		 */
		if (ai->is_empty) {
			ubi->vtbl = create_empty_lvol(ubi, ai);
			if (IS_ERR(ubi->vtbl))
				return PTR_ERR(ubi->vtbl);
		} else {
			ubi_err(ubi, "the layout volume was not found");
			return -EINVAL;
		}
	} else {
		if (av->leb_count > UBI_LAYOUT_VOLUME_EBS) {
			/* This must not happen with proper UBI images */
			ubi_err(ubi, "too many LEBs (%d) in layout volume",
				av->leb_count);
			return -EINVAL;
		}

		ubi->vtbl = process_lvol(ubi, ai, av);
		if (IS_ERR(ubi->vtbl))
			return PTR_ERR(ubi->vtbl);
	}

	ubi->avail_pebs = ubi->good_peb_count - ubi->corr_peb_count;

	/*
	 * The layout volume is OK, initialize the corresponding in-RAM data
	 * structures.
	 */
	err = init_volumes(ubi, ai, ubi->vtbl);
	if (err)
		goto out_free;

	/*
	 * Make sure that the attaching information is consistent to the
	 * information stored in the volume table.
	 */
	err = check_attaching_info(ubi, ai);
	if (err)
		goto out_free;

	return 0;

out_free:
	vfree(ubi->vtbl);
	ubi_free_all_volumes(ubi);
	return err;
}