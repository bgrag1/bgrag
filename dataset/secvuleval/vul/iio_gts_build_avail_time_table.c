static int iio_gts_build_avail_time_table(struct iio_gts *gts)
{
	int *times, i, j, idx = 0, *int_micro_times;

	if (!gts->num_itime)
		return 0;

	times = kcalloc(gts->num_itime, sizeof(int), GFP_KERNEL);
	if (!times)
		return -ENOMEM;

	/* Sort times from all tables to one and remove duplicates */
	for (i = gts->num_itime - 1; i >= 0; i--) {
		int new = gts->itime_table[i].time_us;

		if (times[idx] < new) {
			times[idx++] = new;
			continue;
		}

		for (j = 0; j <= idx; j++) {
			if (times[j] > new) {
				memmove(&times[j + 1], &times[j],
					(idx - j) * sizeof(int));
				times[j] = new;
				idx++;
			}
		}
	}

	/* create a list of times formatted as list of IIO_VAL_INT_PLUS_MICRO */
	int_micro_times = kcalloc(idx, sizeof(int) * 2, GFP_KERNEL);
	if (int_micro_times) {
		/*
		 * This is just to survive a unlikely corner-case where times in
		 * the given time table were not unique. Else we could just
		 * trust the gts->num_itime.
		 */
		gts->num_avail_time_tables = idx;
		iio_gts_us_to_int_micro(times, int_micro_times, idx);
	}

	gts->avail_time_tables = int_micro_times;
	kfree(times);

	if (!int_micro_times)
		return -ENOMEM;

	return 0;
}
