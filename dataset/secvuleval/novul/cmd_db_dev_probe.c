static int cmd_db_dev_probe(struct platform_device *pdev)
{
	struct reserved_mem *rmem;
	int ret = 0;

	rmem = of_reserved_mem_lookup(pdev->dev.of_node);
	if (!rmem) {
		dev_err(&pdev->dev, "failed to acquire memory region\n");
		return -EINVAL;
	}

	cmd_db_header = memremap(rmem->base, rmem->size, MEMREMAP_WC);
	if (!cmd_db_header) {
		ret = -ENOMEM;
		cmd_db_header = NULL;
		return ret;
	}

	if (!cmd_db_magic_matches(cmd_db_header)) {
		dev_err(&pdev->dev, "Invalid Command DB Magic\n");
		return -EINVAL;
	}

	debugfs_create_file("cmd-db", 0400, NULL, NULL, &cmd_db_debugfs_ops);

	device_set_pm_not_required(&pdev->dev);

	return 0;
}
