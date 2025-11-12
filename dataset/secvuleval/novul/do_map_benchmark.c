static int do_map_benchmark(struct map_benchmark_data *map)
{
	struct task_struct **tsk;
	int threads = map->bparam.threads;
	int node = map->bparam.node;
	u64 loops;
	int ret = 0;
	int i;

	tsk = kmalloc_array(threads, sizeof(*tsk), GFP_KERNEL);
	if (!tsk)
		return -ENOMEM;

	get_device(map->dev);

	for (i = 0; i < threads; i++) {
		tsk[i] = kthread_create_on_node(map_benchmark_thread, map,
				map->bparam.node, "dma-map-benchmark/%d", i);
		if (IS_ERR(tsk[i])) {
			pr_err("create dma_map thread failed\n");
			ret = PTR_ERR(tsk[i]);
			while (--i >= 0)
				kthread_stop(tsk[i]);
			goto out;
		}

		if (node != NUMA_NO_NODE)
			kthread_bind_mask(tsk[i], cpumask_of_node(node));
	}

	/* clear the old value in the previous benchmark */
	atomic64_set(&map->sum_map_100ns, 0);
	atomic64_set(&map->sum_unmap_100ns, 0);
	atomic64_set(&map->sum_sq_map, 0);
	atomic64_set(&map->sum_sq_unmap, 0);
	atomic64_set(&map->loops, 0);

	for (i = 0; i < threads; i++) {
		get_task_struct(tsk[i]);
		wake_up_process(tsk[i]);
	}

	msleep_interruptible(map->bparam.seconds * 1000);

	/* wait for the completion of all started benchmark threads */
	for (i = 0; i < threads; i++) {
		int kthread_ret = kthread_stop_put(tsk[i]);

		if (kthread_ret)
			ret = kthread_ret;
	}

	if (ret)
		goto out;

	loops = atomic64_read(&map->loops);
	if (likely(loops > 0)) {
		u64 map_variance, unmap_variance;
		u64 sum_map = atomic64_read(&map->sum_map_100ns);
		u64 sum_unmap = atomic64_read(&map->sum_unmap_100ns);
		u64 sum_sq_map = atomic64_read(&map->sum_sq_map);
		u64 sum_sq_unmap = atomic64_read(&map->sum_sq_unmap);

		/* average latency */
		map->bparam.avg_map_100ns = div64_u64(sum_map, loops);
		map->bparam.avg_unmap_100ns = div64_u64(sum_unmap, loops);

		/* standard deviation of latency */
		map_variance = div64_u64(sum_sq_map, loops) -
				map->bparam.avg_map_100ns *
				map->bparam.avg_map_100ns;
		unmap_variance = div64_u64(sum_sq_unmap, loops) -
				map->bparam.avg_unmap_100ns *
				map->bparam.avg_unmap_100ns;
		map->bparam.map_stddev = int_sqrt64(map_variance);
		map->bparam.unmap_stddev = int_sqrt64(unmap_variance);
	}

out:
	put_device(map->dev);
	kfree(tsk);
	return ret;
}
