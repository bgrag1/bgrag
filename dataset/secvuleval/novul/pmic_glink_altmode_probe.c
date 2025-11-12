static int pmic_glink_altmode_probe(struct auxiliary_device *adev,
				    const struct auxiliary_device_id *id)
{
	struct pmic_glink_altmode_port *alt_port;
	struct pmic_glink_altmode *altmode;
	const struct of_device_id *match;
	struct fwnode_handle *fwnode;
	struct device *dev = &adev->dev;
	u32 port;
	int ret;

	altmode = devm_kzalloc(dev, sizeof(*altmode), GFP_KERNEL);
	if (!altmode)
		return -ENOMEM;

	altmode->dev = dev;

	match = of_match_device(pmic_glink_altmode_of_quirks, dev->parent);
	if (match)
		altmode->owner_id = (unsigned long)match->data;
	else
		altmode->owner_id = PMIC_GLINK_OWNER_USBC_PAN;

	INIT_WORK(&altmode->enable_work, pmic_glink_altmode_enable_worker);
	init_completion(&altmode->pan_ack);
	mutex_init(&altmode->lock);

	device_for_each_child_node(dev, fwnode) {
		ret = fwnode_property_read_u32(fwnode, "reg", &port);
		if (ret < 0) {
			dev_err(dev, "missing reg property of %pOFn\n", fwnode);
			fwnode_handle_put(fwnode);
			return ret;
		}

		if (port >= ARRAY_SIZE(altmode->ports)) {
			dev_warn(dev, "invalid connector number, ignoring\n");
			continue;
		}

		if (altmode->ports[port].altmode) {
			dev_err(dev, "multiple connector definition for port %u\n", port);
			fwnode_handle_put(fwnode);
			return -EINVAL;
		}

		alt_port = &altmode->ports[port];
		alt_port->altmode = altmode;
		alt_port->index = port;
		INIT_WORK(&alt_port->work, pmic_glink_altmode_worker);

		alt_port->bridge = devm_drm_dp_hpd_bridge_alloc(dev, to_of_node(fwnode));
		if (IS_ERR(alt_port->bridge)) {
			fwnode_handle_put(fwnode);
			return PTR_ERR(alt_port->bridge);
		}

		alt_port->dp_alt.svid = USB_TYPEC_DP_SID;
		alt_port->dp_alt.mode = USB_TYPEC_DP_MODE;
		alt_port->dp_alt.active = 1;

		alt_port->typec_mux = fwnode_typec_mux_get(fwnode);
		if (IS_ERR(alt_port->typec_mux)) {
			fwnode_handle_put(fwnode);
			return dev_err_probe(dev, PTR_ERR(alt_port->typec_mux),
					     "failed to acquire mode-switch for port: %d\n",
					     port);
		}

		ret = devm_add_action_or_reset(dev, pmic_glink_altmode_put_mux,
					       alt_port->typec_mux);
		if (ret) {
			fwnode_handle_put(fwnode);
			return ret;
		}

		alt_port->typec_retimer = fwnode_typec_retimer_get(fwnode);
		if (IS_ERR(alt_port->typec_retimer)) {
			fwnode_handle_put(fwnode);
			return dev_err_probe(dev, PTR_ERR(alt_port->typec_retimer),
					     "failed to acquire retimer-switch for port: %d\n",
					     port);
		}

		ret = devm_add_action_or_reset(dev, pmic_glink_altmode_put_retimer,
					       alt_port->typec_retimer);
		if (ret) {
			fwnode_handle_put(fwnode);
			return ret;
		}

		alt_port->typec_switch = fwnode_typec_switch_get(fwnode);
		if (IS_ERR(alt_port->typec_switch)) {
			fwnode_handle_put(fwnode);
			return dev_err_probe(dev, PTR_ERR(alt_port->typec_switch),
					     "failed to acquire orientation-switch for port: %d\n",
					     port);
		}

		ret = devm_add_action_or_reset(dev, pmic_glink_altmode_put_switch,
					       alt_port->typec_switch);
		if (ret) {
			fwnode_handle_put(fwnode);
			return ret;
		}
	}

	for (port = 0; port < ARRAY_SIZE(altmode->ports); port++) {
		alt_port = &altmode->ports[port];
		if (!alt_port->bridge)
			continue;

		ret = devm_drm_dp_hpd_bridge_add(dev, alt_port->bridge);
		if (ret)
			return ret;
	}

	altmode->client = devm_pmic_glink_client_alloc(dev,
						       altmode->owner_id,
						       pmic_glink_altmode_callback,
						       pmic_glink_altmode_pdr_notify,
						       altmode);
	if (IS_ERR(altmode->client))
		return PTR_ERR(altmode->client);

	pmic_glink_client_register(altmode->client);

	return 0;
}
