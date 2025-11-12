int ufshcd_mcq_sq_cleanup(struct ufs_hba *hba, int task_tag)
{
	struct ufshcd_lrb *lrbp = &hba->lrb[task_tag];
	struct scsi_cmnd *cmd = lrbp->cmd;
	struct ufs_hw_queue *hwq;
	void __iomem *reg, *opr_sqd_base;
	u32 nexus, id, val;
	int err;

	if (hba->quirks & UFSHCD_QUIRK_MCQ_BROKEN_RTC)
		return -ETIMEDOUT;

	if (task_tag != hba->nutrs - UFSHCD_NUM_RESERVED) {
		if (!cmd)
			return -EINVAL;
		hwq = ufshcd_mcq_req_to_hwq(hba, scsi_cmd_to_rq(cmd));
		if (!hwq)
			return 0;
	} else {
		hwq = hba->dev_cmd_queue;
	}

	id = hwq->id;

	mutex_lock(&hwq->sq_mutex);

	/* stop the SQ fetching before working on it */
	err = ufshcd_mcq_sq_stop(hba, hwq);
	if (err)
		goto unlock;

	/* SQCTI = EXT_IID, IID, LUN, Task Tag */
	nexus = lrbp->lun << 8 | task_tag;
	opr_sqd_base = mcq_opr_base(hba, OPR_SQD, id);
	writel(nexus, opr_sqd_base + REG_SQCTI);

	/* SQRTCy.ICU = 1 */
	writel(SQ_ICU, opr_sqd_base + REG_SQRTC);

	/* Poll SQRTSy.CUS = 1. Return result from SQRTSy.RTC */
	reg = opr_sqd_base + REG_SQRTS;
	err = read_poll_timeout(readl, val, val & SQ_CUS, 20,
				MCQ_POLL_US, false, reg);
	if (err)
		dev_err(hba->dev, "%s: failed. hwq=%d, tag=%d err=%ld\n",
			__func__, id, task_tag,
			FIELD_GET(SQ_ICU_ERR_CODE_MASK, readl(reg)));

	if (ufshcd_mcq_sq_start(hba, hwq))
		err = -ETIMEDOUT;

unlock:
	mutex_unlock(&hwq->sq_mutex);
	return err;
}
