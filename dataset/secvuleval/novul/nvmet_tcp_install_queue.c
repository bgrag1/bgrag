static u16 nvmet_tcp_install_queue(struct nvmet_sq *sq)
{
	struct nvmet_tcp_queue *queue =
		container_of(sq, struct nvmet_tcp_queue, nvme_sq);

	if (sq->qid == 0) {
		struct nvmet_tcp_queue *q;
		int pending = 0;

		/* Check for pending controller teardown */
		mutex_lock(&nvmet_tcp_queue_mutex);
		list_for_each_entry(q, &nvmet_tcp_queue_list, queue_list) {
			if (q->nvme_sq.ctrl == sq->ctrl &&
			    q->state == NVMET_TCP_Q_DISCONNECTING)
				pending++;
		}
		mutex_unlock(&nvmet_tcp_queue_mutex);
		if (pending > NVMET_TCP_BACKLOG)
			return NVME_SC_CONNECT_CTRL_BUSY;
	}

	queue->nr_cmds = sq->size * 2;
	if (nvmet_tcp_alloc_cmds(queue)) {
		queue->nr_cmds = 0;
		return NVME_SC_INTERNAL;
	}
	return 0;
}
