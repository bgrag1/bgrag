static int ublk_ctrl_start_recovery(struct ublk_device *ub,
		struct io_uring_cmd *cmd)
{
	const struct ublksrv_ctrl_cmd *header = io_uring_sqe_cmd(cmd->sqe);
	int ret = -EINVAL;
	int i;

	mutex_lock(&ub->mutex);
	if (!ublk_can_use_recovery(ub))
		goto out_unlock;
	/*
	 * START_RECOVERY is only allowd after:
	 *
	 * (1) UB_STATE_OPEN is not set, which means the dying process is exited
	 *     and related io_uring ctx is freed so file struct of /dev/ublkcX is
	 *     released.
	 *
	 * (2) UBLK_S_DEV_QUIESCED is set, which means the quiesce_work:
	 *     (a)has quiesced request queue
	 *     (b)has requeued every inflight rqs whose io_flags is ACTIVE
	 *     (c)has requeued/aborted every inflight rqs whose io_flags is NOT ACTIVE
	 *     (d)has completed/camceled all ioucmds owned by ther dying process
	 */
	if (test_bit(UB_STATE_OPEN, &ub->state) ||
			ub->dev_info.state != UBLK_S_DEV_QUIESCED) {
		ret = -EBUSY;
		goto out_unlock;
	}
	pr_devel("%s: start recovery for dev id %d.\n", __func__, header->dev_id);
	for (i = 0; i < ub->dev_info.nr_hw_queues; i++)
		ublk_queue_reinit(ub, ublk_get_queue(ub, i));
	/* set to NULL, otherwise new ubq_daemon cannot mmap the io_cmd_buf */
	ub->mm = NULL;
	ub->nr_queues_ready = 0;
	ub->nr_privileged_daemon = 0;
	init_completion(&ub->completion);
	ret = 0;
 out_unlock:
	mutex_unlock(&ub->mutex);
	return ret;
}
