struct ufs_hw_queue *ufshcd_mcq_req_to_hwq(struct ufs_hba *hba,
					 struct request *req)
{
	u32 utag = blk_mq_unique_tag(req);
	u32 hwq = blk_mq_unique_tag_to_hwq(utag);

	return &hba->uhq[hwq];
}
