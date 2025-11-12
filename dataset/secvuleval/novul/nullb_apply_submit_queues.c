static int nullb_apply_submit_queues(struct nullb_device *dev,
				     unsigned int submit_queues)
{
	int ret;

	mutex_lock(&lock);
	ret = nullb_update_nr_hw_queues(dev, submit_queues, dev->poll_queues);
	mutex_unlock(&lock);

	return ret;
}
