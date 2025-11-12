static int fastrpc_req_mmap(struct fastrpc_user *fl, char __user *argp)
{
	struct fastrpc_invoke_args args[3] = { [0 ... 2] = { 0 } };
	struct fastrpc_buf *buf = NULL;
	struct fastrpc_mmap_req_msg req_msg;
	struct fastrpc_mmap_rsp_msg rsp_msg;
	struct fastrpc_phy_page pages;
	struct fastrpc_req_mmap req;
	struct device *dev = fl->sctx->dev;
	int err;
	u32 sc;

	if (copy_from_user(&req, argp, sizeof(req)))
		return -EFAULT;

	if (req.flags != ADSP_MMAP_ADD_PAGES && req.flags != ADSP_MMAP_REMOTE_HEAP_ADDR) {
		dev_err(dev, "flag not supported 0x%x\n", req.flags);

		return -EINVAL;
	}

	if (req.vaddrin) {
		dev_err(dev, "adding user allocated pages is not supported\n");
		return -EINVAL;
	}

	if (req.flags == ADSP_MMAP_REMOTE_HEAP_ADDR)
		err = fastrpc_remote_heap_alloc(fl, dev, req.size, &buf);
	else
		err = fastrpc_buf_alloc(fl, dev, req.size, &buf);

	if (err) {
		dev_err(dev, "failed to allocate buffer\n");
		return err;
	}

	req_msg.pgid = fl->tgid;
	req_msg.flags = req.flags;
	req_msg.vaddr = req.vaddrin;
	req_msg.num = sizeof(pages);

	args[0].ptr = (u64) (uintptr_t) &req_msg;
	args[0].length = sizeof(req_msg);

	pages.addr = buf->phys;
	pages.size = buf->size;

	args[1].ptr = (u64) (uintptr_t) &pages;
	args[1].length = sizeof(pages);

	args[2].ptr = (u64) (uintptr_t) &rsp_msg;
	args[2].length = sizeof(rsp_msg);

	sc = FASTRPC_SCALARS(FASTRPC_RMID_INIT_MMAP, 2, 1);
	err = fastrpc_internal_invoke(fl, true, FASTRPC_INIT_HANDLE, sc,
				      &args[0]);
	if (err) {
		dev_err(dev, "mmap error (len 0x%08llx)\n", buf->size);
		goto err_invoke;
	}

	/* update the buffer to be able to deallocate the memory on the DSP */
	buf->raddr = (uintptr_t) rsp_msg.vaddr;

	/* let the client know the address to use */
	req.vaddrout = rsp_msg.vaddr;

	/* Add memory to static PD pool, protection thru hypervisor */
	if (req.flags == ADSP_MMAP_REMOTE_HEAP_ADDR && fl->cctx->vmcount) {
		u64 src_perms = BIT(QCOM_SCM_VMID_HLOS);

		err = qcom_scm_assign_mem(buf->phys, (u64)buf->size,
			&src_perms, fl->cctx->vmperms, fl->cctx->vmcount);
		if (err) {
			dev_err(fl->sctx->dev, "Failed to assign memory phys 0x%llx size 0x%llx err %d",
					buf->phys, buf->size, err);
			goto err_assign;
		}
	}

	spin_lock(&fl->lock);
	list_add_tail(&buf->node, &fl->mmaps);
	spin_unlock(&fl->lock);

	if (copy_to_user((void __user *)argp, &req, sizeof(req))) {
		err = -EFAULT;
		goto err_assign;
	}

	dev_dbg(dev, "mmap\t\tpt 0x%09lx OK [len 0x%08llx]\n",
		buf->raddr, buf->size);

	return 0;

err_assign:
	fastrpc_req_munmap_impl(fl, buf);
err_invoke:
	fastrpc_buf_free(buf);

	return err;
}
