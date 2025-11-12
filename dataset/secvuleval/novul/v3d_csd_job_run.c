static struct dma_fence *
v3d_csd_job_run(struct drm_sched_job *sched_job)
{
	struct v3d_csd_job *job = to_csd_job(sched_job);
	struct v3d_dev *v3d = job->base.v3d;
	struct drm_device *dev = &v3d->drm;
	struct dma_fence *fence;
	int i, csd_cfg0_reg;

	v3d->csd_job = job;

	v3d_invalidate_caches(v3d);

	fence = v3d_fence_create(v3d, V3D_CSD);
	if (IS_ERR(fence))
		return NULL;

	if (job->base.irq_fence)
		dma_fence_put(job->base.irq_fence);
	job->base.irq_fence = dma_fence_get(fence);

	trace_v3d_submit_csd(dev, to_v3d_fence(fence)->seqno);

	v3d_job_start_stats(&job->base, V3D_CSD);
	v3d_switch_perfmon(v3d, &job->base);

	csd_cfg0_reg = V3D_CSD_QUEUED_CFG0(v3d->ver);
	for (i = 1; i <= 6; i++)
		V3D_CORE_WRITE(0, csd_cfg0_reg + 4 * i, job->args.cfg[i]);

	/* Although V3D 7.1 has an eighth configuration register, we are not
	 * using it. Therefore, make sure it remains unused.
	 *
	 * XXX: Set the CFG7 register
	 */
	if (v3d->ver >= 71)
		V3D_CORE_WRITE(0, V3D_V7_CSD_QUEUED_CFG7, 0);

	/* CFG0 write kicks off the job. */
	V3D_CORE_WRITE(0, csd_cfg0_reg, job->args.cfg[0]);

	return fence;
}
