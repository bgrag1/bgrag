static int ssb_device_uevent(const struct device *dev, struct kobj_uevent_env *env)
{
	const struct ssb_device *ssb_dev;

	if (!dev)
		return -ENODEV;

	ssb_dev = dev_to_ssb_dev(dev);

	return add_uevent_var(env,
			     "MODALIAS=ssb:v%04Xid%04Xrev%02X",
			     ssb_dev->id.vendor, ssb_dev->id.coreid,
			     ssb_dev->id.revision);
}
