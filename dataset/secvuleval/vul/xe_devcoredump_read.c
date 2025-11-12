static ssize_t xe_devcoredump_read(char *buffer, loff_t offset,
				   size_t count, void *data, size_t datalen)
{
	struct xe_devcoredump *coredump = data;
	struct xe_device *xe = coredump_to_xe(coredump);
	struct xe_devcoredump_snapshot *ss = &coredump->snapshot;
	struct drm_printer p;
	struct drm_print_iterator iter;
	struct timespec64 ts;
	int i;

	/* Our device is gone already... */
	if (!data || !coredump_to_xe(coredump))
		return -ENODEV;

	/* Ensure delayed work is captured before continuing */
	flush_work(&ss->work);

	iter.data = buffer;
	iter.offset = 0;
	iter.start = offset;
	iter.remain = count;

	p = drm_coredump_printer(&iter);

	drm_printf(&p, "**** Xe Device Coredump ****\n");
	drm_printf(&p, "kernel: " UTS_RELEASE "\n");
	drm_printf(&p, "module: " KBUILD_MODNAME "\n");

	ts = ktime_to_timespec64(ss->snapshot_time);
	drm_printf(&p, "Snapshot time: %lld.%09ld\n", ts.tv_sec, ts.tv_nsec);
	ts = ktime_to_timespec64(ss->boot_time);
	drm_printf(&p, "Uptime: %lld.%09ld\n", ts.tv_sec, ts.tv_nsec);
	xe_device_snapshot_print(xe, &p);

	drm_printf(&p, "\n**** GuC CT ****\n");
	xe_guc_ct_snapshot_print(coredump->snapshot.ct, &p);
	xe_guc_exec_queue_snapshot_print(coredump->snapshot.ge, &p);

	drm_printf(&p, "\n**** Job ****\n");
	xe_sched_job_snapshot_print(coredump->snapshot.job, &p);

	drm_printf(&p, "\n**** HW Engines ****\n");
	for (i = 0; i < XE_NUM_HW_ENGINES; i++)
		if (coredump->snapshot.hwe[i])
			xe_hw_engine_snapshot_print(coredump->snapshot.hwe[i],
						    &p);
	drm_printf(&p, "\n**** VM state ****\n");
	xe_vm_snapshot_print(coredump->snapshot.vm, &p);

	return count - iter.remain;
}
