static int cxl_pmem_region_alloc(struct cxl_region *cxlr)
{
	struct cxl_region_params *p = &cxlr->params;
	struct cxl_nvdimm_bridge *cxl_nvb;
	struct device *dev;
	int i;

	guard(rwsem_read)(&cxl_region_rwsem);
	if (p->state != CXL_CONFIG_COMMIT)
		return -ENXIO;

	struct cxl_pmem_region *cxlr_pmem __free(kfree) =
		kzalloc(struct_size(cxlr_pmem, mapping, p->nr_targets), GFP_KERNEL);
	if (!cxlr_pmem)
		return -ENOMEM;

	cxlr_pmem->hpa_range.start = p->res->start;
	cxlr_pmem->hpa_range.end = p->res->end;

	/* Snapshot the region configuration underneath the cxl_region_rwsem */
	cxlr_pmem->nr_mappings = p->nr_targets;
	for (i = 0; i < p->nr_targets; i++) {
		struct cxl_endpoint_decoder *cxled = p->targets[i];
		struct cxl_memdev *cxlmd = cxled_to_memdev(cxled);
		struct cxl_pmem_region_mapping *m = &cxlr_pmem->mapping[i];

		/*
		 * Regions never span CXL root devices, so by definition the
		 * bridge for one device is the same for all.
		 */
		if (i == 0) {
			cxl_nvb = cxl_find_nvdimm_bridge(cxlmd);
			if (!cxl_nvb)
				return -ENODEV;
			cxlr->cxl_nvb = cxl_nvb;
		}
		m->cxlmd = cxlmd;
		get_device(&cxlmd->dev);
		m->start = cxled->dpa_res->start;
		m->size = resource_size(cxled->dpa_res);
		m->position = i;
	}

	dev = &cxlr_pmem->dev;
	device_initialize(dev);
	lockdep_set_class(&dev->mutex, &cxl_pmem_region_key);
	device_set_pm_not_required(dev);
	dev->parent = &cxlr->dev;
	dev->bus = &cxl_bus_type;
	dev->type = &cxl_pmem_region_type;
	cxlr_pmem->cxlr = cxlr;
	cxlr->cxlr_pmem = no_free_ptr(cxlr_pmem);

	return 0;
}
