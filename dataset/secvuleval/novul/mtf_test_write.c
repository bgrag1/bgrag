static ssize_t mtf_test_write(struct file *file, const char __user *buf,
	size_t count, loff_t *pos)
{
	struct seq_file *sf = file->private_data;
	struct mmc_card *card = sf->private;
	struct mmc_test_card *test;
	long testcase;
	int ret;

	ret = kstrtol_from_user(buf, count, 10, &testcase);
	if (ret)
		return ret;

	test = kzalloc(sizeof(*test), GFP_KERNEL);
	if (!test)
		return -ENOMEM;

	/*
	 * Remove all test cases associated with given card. Thus we have only
	 * actual data of the last run.
	 */
	mmc_test_free_result(card);

	test->card = card;

	test->buffer = kzalloc(BUFFER_SIZE, GFP_KERNEL);
// #ifdef CONFIG_HIGHMEM
	test->highmem = alloc_pages(GFP_KERNEL | __GFP_HIGHMEM, BUFFER_ORDER);
	if (!test->highmem) {
		count = -ENOMEM;
		goto free_test_buffer;
	}
#endif

	if (test->buffer) {
		mutex_lock(&mmc_test_lock);
		mmc_test_run(test, testcase);
		mutex_unlock(&mmc_test_lock);
	}

// #ifdef CONFIG_HIGHMEM
	__free_pages(test->highmem, BUFFER_ORDER);
free_test_buffer:
#endif
	kfree(test->buffer);
	kfree(test);

	return count;
}
