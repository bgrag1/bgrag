static int pmic_glink_ucsi_probe(struct auxiliary_device *adev,
				 const struct auxiliary_device_id *id)
{
	struct pmic_glink_ucsi *ucsi;
	struct device *dev = &adev->dev;
	const struct of_device_id *match;
	struct fwnode_handle *fwnode;
	int ret;

	ucsi = devm_kzalloc(dev, sizeof(*ucsi), GFP_KERNEL);
	if (!ucsi)
		return -ENOMEM;

	ucsi->dev = dev;
	dev_set_drvdata(dev, ucsi);

	INIT_WORK(&ucsi->notify_work, pmic_glink_ucsi_notify);
	INIT_WORK(&ucsi->register_work, pmic_glink_ucsi_register);
	init_completion(&ucsi->read_ack);
	init_completion(&ucsi->write_ack);
	mutex_init(&ucsi->lock);

	ucsi->ucsi = ucsi_create(dev, &pmic_glink_ucsi_ops);
	if (IS_ERR(ucsi->ucsi))
		return PTR_ERR(ucsi->ucsi);

	/* Make sure we destroy *after* pmic_glink unregister */
	ret = devm_add_action_or_reset(dev, pmic_glink_ucsi_destroy, ucsi);
	if (ret)
		return ret;

	match = of_match_device(pmic_glink_ucsi_of_quirks, dev->parent);
	if (match)
		ucsi->ucsi->quirks = *(unsigned long *)match->data;

	ucsi_set_drvdata(ucsi->ucsi, ucsi);

	device_for_each_child_node(dev, fwnode) {
		struct gpio_desc *desc;
		u32 port;

		ret = fwnode_property_read_u32(fwnode, "reg", &port);
		if (ret < 0) {
			dev_err(dev, "missing reg property of %pOFn\n", fwnode);
			fwnode_handle_put(fwnode);
			return ret;
		}

		if (port >= PMIC_GLINK_MAX_PORTS) {
			dev_warn(dev, "invalid connector number, ignoring\n");
			continue;
		}

		desc = devm_gpiod_get_index_optional(&adev->dev, "orientation", port, GPIOD_IN);

		/* If GPIO isn't found, continue */
		if (!desc)
			continue;

		if (IS_ERR(desc)) {
			fwnode_handle_put(fwnode);
			return dev_err_probe(dev, PTR_ERR(desc),
					     "unable to acquire orientation gpio\n");
		}
		ucsi->port_orientation[port] = desc;
	}

	ucsi->client = devm_pmic_glink_register_client(dev,
						       PMIC_GLINK_OWNER_USBC,
						       pmic_glink_ucsi_callback,
						       pmic_glink_ucsi_pdr_notify,
						       ucsi);
	return PTR_ERR_OR_ZERO(ucsi->client);
}
