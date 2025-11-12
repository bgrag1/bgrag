static void cs35l41_hda_unbind(struct device *dev, struct device *master, void *master_data)
{
	struct cs35l41_hda *cs35l41 = dev_get_drvdata(dev);
	struct hda_component *comps = master_data;
	unsigned int sleep_flags;

	if (comps[cs35l41->index].dev == dev) {
		memset(&comps[cs35l41->index], 0, sizeof(*comps));
		sleep_flags = lock_system_sleep();
		device_link_remove(&comps->codec->core.dev, cs35l41->dev);
		unlock_system_sleep(sleep_flags);
	}
}
