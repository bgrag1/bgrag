struct ufs_hw_queue *ufshcd_mcq_req_to_hwq(struct ufs_hba *hba,
					 struct request *req)
{
	struct blk_mq_hw_ctx *hctx = READ_ONCE(req->mq_hctx);

	return hctx ? &hba->uhq[hctx->queue_num] : NULL;
}
