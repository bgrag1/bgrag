static int __cxl_dpa_to_region(struct device *dev, void *arg)
{
	struct cxl_dpa_to_region_context *ctx = arg;
	struct cxl_endpoint_decoder *cxled;
	u64 dpa = ctx->dpa;

	if (!is_endpoint_decoder(dev))
		return 0;

	cxled = to_cxl_endpoint_decoder(dev);
	if (!cxled->dpa_res || !resource_size(cxled->dpa_res))
		return 0;

	if (dpa > cxled->dpa_res->end || dpa < cxled->dpa_res->start)
		return 0;

	dev_dbg(dev, "dpa:0x%llx mapped in region:%s\n", dpa,
		dev_name(&cxled->cxld.region->dev));

	ctx->cxlr = cxled->cxld.region;

	return 1;
}
