void nvme_cleanup_cmd(struct request *req)
{
	if (req->rq_flags & RQF_SPECIAL_PAYLOAD) {
		struct nvme_ctrl *ctrl = nvme_req(req)->ctrl;

		if (req->special_vec.bv_page == ctrl->discard_page)
			clear_bit_unlock(0, &ctrl->discard_page_busy);
		else
			kfree(bvec_virt(&req->special_vec));
		req->rq_flags &= ~RQF_SPECIAL_PAYLOAD;
	}
}
