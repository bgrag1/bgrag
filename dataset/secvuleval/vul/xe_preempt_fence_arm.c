struct dma_fence *
xe_preempt_fence_arm(struct xe_preempt_fence *pfence, struct xe_exec_queue *q,
		     u64 context, u32 seqno)
{
	list_del_init(&pfence->link);
	pfence->q = xe_exec_queue_get(q);
	dma_fence_init(&pfence->base, &preempt_fence_ops,
		      &q->lr.lock, context, seqno);

	return &pfence->base;
}
