void xe_sched_job_destroy(struct kref *ref)
{
	struct xe_sched_job *job =
		container_of(ref, struct xe_sched_job, refcount);
	struct xe_device *xe = job_to_xe(job);
	struct xe_exec_queue *q = job->q;

	xe_sched_job_free_fences(job);
	dma_fence_put(job->fence);
	drm_sched_job_cleanup(&job->drm);
	job_free(job);
	xe_exec_queue_put(q);
	xe_pm_runtime_put(xe);
}
