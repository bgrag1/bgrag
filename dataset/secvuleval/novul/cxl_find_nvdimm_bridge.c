struct cxl_nvdimm_bridge *cxl_find_nvdimm_bridge(struct cxl_port *port)
{
	struct cxl_root *cxl_root __free(put_cxl_root) = find_cxl_root(port);
	struct device *dev;

	if (!cxl_root)
		return NULL;

	dev = device_find_child(&cxl_root->port.dev, NULL, match_nvdimm_bridge);

	if (!dev)
		return NULL;

	return to_cxl_nvdimm_bridge(dev);
}
