static struct bfq_queue *bfq_waker_bfqq(struct bfq_queue *bfqq)
{
	struct bfq_queue *new_bfqq = bfqq->new_bfqq;
	struct bfq_queue *waker_bfqq = bfqq->waker_bfqq;

	if (!waker_bfqq)
		return NULL;

	while (new_bfqq) {
		if (new_bfqq == waker_bfqq) {
			/*
			 * If waker_bfqq is in the merge chain, and current
			 * is the only procress.
			 */
			if (bfqq_process_refs(waker_bfqq) == 1)
				return NULL;
			break;
		}

		new_bfqq = new_bfqq->new_bfqq;
	}

	return waker_bfqq;
}
