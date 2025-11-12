static void gb_interface_release(struct device *dev)
{
	struct gb_interface *intf = to_gb_interface(dev);

	trace_gb_interface_release(intf);

	kfree(intf);
}
