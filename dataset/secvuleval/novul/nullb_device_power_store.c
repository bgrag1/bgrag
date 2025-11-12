static ssize_t nullb_device_power_store(struct config_item *item,
				     const char *page, size_t count)
{
	struct nullb_device *dev = to_nullb_device(item);
	bool newp = false;
	ssize_t ret;

	ret = nullb_device_bool_attr_store(&newp, page, count);
	if (ret < 0)
		return ret;

	ret = count;
	mutex_lock(&lock);
	if (!dev->power && newp) {
		if (test_and_set_bit(NULLB_DEV_FL_UP, &dev->flags))
			goto out;

		ret = null_add_dev(dev);
		if (ret) {
			clear_bit(NULLB_DEV_FL_UP, &dev->flags);
			goto out;
		}

		set_bit(NULLB_DEV_FL_CONFIGURED, &dev->flags);
		dev->power = newp;
	} else if (dev->power && !newp) {
		if (test_and_clear_bit(NULLB_DEV_FL_UP, &dev->flags)) {
			dev->power = newp;
			null_del_dev(dev->nullb);
		}
		clear_bit(NULLB_DEV_FL_CONFIGURED, &dev->flags);
	}

out:
	mutex_unlock(&lock);
	return ret;
}
