void rdma_restrack_clean(struct ib_device *dev)
{
	struct rdma_restrack_root *rt = dev->res;
	struct rdma_restrack_entry *e;
	char buf[TASK_COMM_LEN];
	bool found = false;
	const char *owner;
	int i;

	for (i = 0 ; i < RDMA_RESTRACK_MAX; i++) {
		struct xarray *xa = &dev->res[i].xa;

		if (!xa_empty(xa)) {
			unsigned long index;

			if (!found) {
				pr_err("restrack: %s", CUT_HERE);
				dev_err(&dev->dev, "BUG: RESTRACK detected leak of resources\n");
			}
			xa_for_each(xa, index, e) {
				if (rdma_is_kernel_res(e)) {
					owner = e->kern_name;
				} else {
					/*
					 * There is no need to call get_task_struct here,
					 * because we can be here only if there are more
					 * get_task_struct() call than put_task_struct().
					 */
					get_task_comm(buf, e->task);
					owner = buf;
				}

				pr_err("restrack: %s %s object allocated by %s is not freed\n",
				       rdma_is_kernel_res(e) ? "Kernel" :
							       "User",
				       type2str(e->type), owner);
			}
			found = true;
		}
		xa_destroy(xa);
	}
	if (found)
		pr_err("restrack: %s", CUT_HERE);

	kfree(rt);
}
