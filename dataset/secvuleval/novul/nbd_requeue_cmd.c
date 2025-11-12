static void nbd_requeue_cmd(struct nbd_cmd *cmd)
{
	struct request *req = blk_mq_rq_from_pdu(cmd);

	lockdep_assert_held(&cmd->lock);

	/*
	 * Clear INFLIGHT flag so that this cmd won't be completed in
	 * normal completion path
	 *
	 * INFLIGHT flag will be set when the cmd is queued to nbd next
	 * time.
	 */
	__clear_bit(NBD_CMD_INFLIGHT, &cmd->flags);

	if (!test_and_set_bit(NBD_CMD_REQUEUED, &cmd->flags))
		blk_mq_requeue_request(req, true);
}
