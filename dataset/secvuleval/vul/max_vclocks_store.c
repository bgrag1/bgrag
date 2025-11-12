static ssize_t max_vclocks_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct ptp_clock *ptp = dev_get_drvdata(dev);
	unsigned int *vclock_index;
	int err = -EINVAL;
	size_t size;
	u32 max;

	if (kstrtou32(buf, 0, &max) || max == 0)
		return -EINVAL;

	if (max == ptp->max_vclocks)
		return count;

	if (mutex_lock_interruptible(&ptp->n_vclocks_mux))
		return -ERESTARTSYS;

	if (max < ptp->n_vclocks)
		goto out;

	size = sizeof(int) * max;
	vclock_index = kzalloc(size, GFP_KERNEL);
	if (!vclock_index) {
		err = -ENOMEM;
		goto out;
	}

	size = sizeof(int) * ptp->n_vclocks;
	memcpy(vclock_index, ptp->vclock_index, size);

	kfree(ptp->vclock_index);
	ptp->vclock_index = vclock_index;
	ptp->max_vclocks = max;

	mutex_unlock(&ptp->n_vclocks_mux);

	return count;
out:
	mutex_unlock(&ptp->n_vclocks_mux);
	return err;
}
