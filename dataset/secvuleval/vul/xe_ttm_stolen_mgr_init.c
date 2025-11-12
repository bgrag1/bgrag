void xe_ttm_stolen_mgr_init(struct xe_device *xe)
{
	struct xe_ttm_stolen_mgr *mgr = drmm_kzalloc(&xe->drm, sizeof(*mgr), GFP_KERNEL);
	struct pci_dev *pdev = to_pci_dev(xe->drm.dev);
	u64 stolen_size, io_size, pgsize;
	int err;

	if (IS_SRIOV_VF(xe))
		stolen_size = 0;
	else if (IS_DGFX(xe))
		stolen_size = detect_bar2_dgfx(xe, mgr);
	else if (GRAPHICS_VERx100(xe) >= 1270)
		stolen_size = detect_bar2_integrated(xe, mgr);
	else
		stolen_size = detect_stolen(xe, mgr);

	if (!stolen_size) {
		drm_dbg_kms(&xe->drm, "No stolen memory support\n");
		return;
	}

	pgsize = xe->info.vram_flags & XE_VRAM_FLAGS_NEED64K ? SZ_64K : SZ_4K;
	if (pgsize < PAGE_SIZE)
		pgsize = PAGE_SIZE;

	/*
	 * We don't try to attempt partial visible support for stolen vram,
	 * since stolen is always at the end of vram, and the BAR size is pretty
	 * much always 256M, with small-bar.
	 */
	io_size = 0;
	if (mgr->io_base && !xe_ttm_stolen_cpu_access_needs_ggtt(xe))
		io_size = stolen_size;

	err = __xe_ttm_vram_mgr_init(xe, &mgr->base, XE_PL_STOLEN, stolen_size,
				     io_size, pgsize);
	if (err) {
		drm_dbg_kms(&xe->drm, "Stolen mgr init failed: %i\n", err);
		return;
	}

	drm_dbg_kms(&xe->drm, "Initialized stolen memory support with %llu bytes\n",
		    stolen_size);

	if (io_size)
		mgr->mapping = devm_ioremap_wc(&pdev->dev, mgr->io_base, io_size);
}
