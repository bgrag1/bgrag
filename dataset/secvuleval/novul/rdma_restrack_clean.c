void rdma_restrack_clean(struct ib_device *dev)
{
	struct rdma_restrack_root *rt = dev->res;
	int i;

	for (i = 0 ; i < RDMA_RESTRACK_MAX; i++) {
		struct xarray *xa = &dev->res[i].xa;

		WARN_ON(!xa_empty(xa));
		xa_destroy(xa);
	}
	kfree(rt);
}
