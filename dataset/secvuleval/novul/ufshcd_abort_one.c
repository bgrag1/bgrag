static bool ufshcd_abort_one(struct request *rq, void *priv)
{
	int *ret = priv;
	u32 tag = rq->tag;
	struct scsi_cmnd *cmd = blk_mq_rq_to_pdu(rq);
	struct scsi_device *sdev = cmd->device;
	struct Scsi_Host *shost = sdev->host;
	struct ufs_hba *hba = shost_priv(shost);
	struct ufshcd_lrb *lrbp = &hba->lrb[tag];
	struct ufs_hw_queue *hwq;
	unsigned long flags;

	*ret = ufshcd_try_to_abort_task(hba, tag);
	dev_err(hba->dev, "Aborting tag %d / CDB %#02x %s\n", tag,
		hba->lrb[tag].cmd ? hba->lrb[tag].cmd->cmnd[0] : -1,
		*ret ? "failed" : "succeeded");

	/* Release cmd in MCQ mode if abort succeeds */
	if (is_mcq_enabled(hba) && (*ret == 0)) {
		hwq = ufshcd_mcq_req_to_hwq(hba, scsi_cmd_to_rq(lrbp->cmd));
		if (!hwq)
			return 0;
		spin_lock_irqsave(&hwq->cq_lock, flags);
		if (ufshcd_cmd_inflight(lrbp->cmd))
			ufshcd_release_scsi_cmd(hba, lrbp);
		spin_unlock_irqrestore(&hwq->cq_lock, flags);
	}

	return *ret == 0;
}
