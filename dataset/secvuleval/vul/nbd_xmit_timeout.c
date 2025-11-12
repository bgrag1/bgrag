static enum blk_eh_timer_return nbd_xmit_timeout(struct request *req)
{
	struct nbd_cmd *cmd = blk_mq_rq_to_pdu(req);
	struct nbd_device *nbd = cmd->nbd;
	struct nbd_config *config;

	if (!mutex_trylock(&cmd->lock))
		return BLK_EH_RESET_TIMER;

	if (!test_bit(NBD_CMD_INFLIGHT, &cmd->flags)) {
		mutex_unlock(&cmd->lock);
		return BLK_EH_DONE;
	}

	config = nbd_get_config_unlocked(nbd);
	if (!config) {
		cmd->status = BLK_STS_TIMEOUT;
		__clear_bit(NBD_CMD_INFLIGHT, &cmd->flags);
		mutex_unlock(&cmd->lock);
		goto done;
	}

	if (config->num_connections > 1 ||
	    (config->num_connections == 1 && nbd->tag_set.timeout)) {
		dev_err_ratelimited(nbd_to_dev(nbd),
				    "Connection timed out, retrying (%d/%d alive)\n",
				    atomic_read(&config->live_connections),
				    config->num_connections);
		/*
		 * Hooray we have more connections, requeue this IO, the submit
		 * path will put it on a real connection. Or if only one
		 * connection is configured, the submit path will wait util
		 * a new connection is reconfigured or util dead timeout.
		 */
		if (config->socks) {
			if (cmd->index < config->num_connections) {
				struct nbd_sock *nsock =
					config->socks[cmd->index];
				mutex_lock(&nsock->tx_lock);
				/* We can have multiple outstanding requests, so
				 * we don't want to mark the nsock dead if we've
				 * already reconnected with a new socket, so
				 * only mark it dead if its the same socket we
				 * were sent out on.
				 */
				if (cmd->cookie == nsock->cookie)
					nbd_mark_nsock_dead(nbd, nsock, 1);
				mutex_unlock(&nsock->tx_lock);
			}
			mutex_unlock(&cmd->lock);
			nbd_requeue_cmd(cmd);
			nbd_config_put(nbd);
			return BLK_EH_DONE;
		}
	}

	if (!nbd->tag_set.timeout) {
		/*
		 * Userspace sets timeout=0 to disable socket disconnection,
		 * so just warn and reset the timer.
		 */
		struct nbd_sock *nsock = config->socks[cmd->index];
		cmd->retries++;
		dev_info(nbd_to_dev(nbd), "Possible stuck request %p: control (%s@%llu,%uB). Runtime %u seconds\n",
			req, nbdcmd_to_ascii(req_to_nbd_cmd_type(req)),
			(unsigned long long)blk_rq_pos(req) << 9,
			blk_rq_bytes(req), (req->timeout / HZ) * cmd->retries);

		mutex_lock(&nsock->tx_lock);
		if (cmd->cookie != nsock->cookie) {
			nbd_requeue_cmd(cmd);
			mutex_unlock(&nsock->tx_lock);
			mutex_unlock(&cmd->lock);
			nbd_config_put(nbd);
			return BLK_EH_DONE;
		}
		mutex_unlock(&nsock->tx_lock);
		mutex_unlock(&cmd->lock);
		nbd_config_put(nbd);
		return BLK_EH_RESET_TIMER;
	}

	dev_err_ratelimited(nbd_to_dev(nbd), "Connection timed out\n");
	set_bit(NBD_RT_TIMEDOUT, &config->runtime_flags);
	cmd->status = BLK_STS_IOERR;
	__clear_bit(NBD_CMD_INFLIGHT, &cmd->flags);
	mutex_unlock(&cmd->lock);
	sock_shutdown(nbd);
	nbd_config_put(nbd);
done:
	blk_mq_complete_request(req);
	return BLK_EH_DONE;
}
