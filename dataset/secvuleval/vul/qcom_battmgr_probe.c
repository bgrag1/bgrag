static int qcom_battmgr_probe(struct auxiliary_device *adev,
			      const struct auxiliary_device_id *id)
{
	struct power_supply_config psy_cfg_supply = {};
	struct power_supply_config psy_cfg = {};
	const struct of_device_id *match;
	struct qcom_battmgr *battmgr;
	struct device *dev = &adev->dev;

	battmgr = devm_kzalloc(dev, sizeof(*battmgr), GFP_KERNEL);
	if (!battmgr)
		return -ENOMEM;

	battmgr->dev = dev;

	psy_cfg.drv_data = battmgr;
	psy_cfg.of_node = adev->dev.of_node;

	psy_cfg_supply.drv_data = battmgr;
	psy_cfg_supply.of_node = adev->dev.of_node;
	psy_cfg_supply.supplied_to = qcom_battmgr_battery;
	psy_cfg_supply.num_supplicants = 1;

	INIT_WORK(&battmgr->enable_work, qcom_battmgr_enable_worker);
	mutex_init(&battmgr->lock);
	init_completion(&battmgr->ack);

	match = of_match_device(qcom_battmgr_of_variants, dev->parent);
	if (match)
		battmgr->variant = (unsigned long)match->data;
	else
		battmgr->variant = QCOM_BATTMGR_SM8350;

	if (battmgr->variant == QCOM_BATTMGR_SC8280XP) {
		battmgr->bat_psy = devm_power_supply_register(dev, &sc8280xp_bat_psy_desc, &psy_cfg);
		if (IS_ERR(battmgr->bat_psy))
			return dev_err_probe(dev, PTR_ERR(battmgr->bat_psy),
					     "failed to register battery power supply\n");

		battmgr->ac_psy = devm_power_supply_register(dev, &sc8280xp_ac_psy_desc, &psy_cfg_supply);
		if (IS_ERR(battmgr->ac_psy))
			return dev_err_probe(dev, PTR_ERR(battmgr->ac_psy),
					     "failed to register AC power supply\n");

		battmgr->usb_psy = devm_power_supply_register(dev, &sc8280xp_usb_psy_desc, &psy_cfg_supply);
		if (IS_ERR(battmgr->usb_psy))
			return dev_err_probe(dev, PTR_ERR(battmgr->usb_psy),
					     "failed to register USB power supply\n");

		battmgr->wls_psy = devm_power_supply_register(dev, &sc8280xp_wls_psy_desc, &psy_cfg_supply);
		if (IS_ERR(battmgr->wls_psy))
			return dev_err_probe(dev, PTR_ERR(battmgr->wls_psy),
					     "failed to register wireless charing power supply\n");
	} else {
		battmgr->bat_psy = devm_power_supply_register(dev, &sm8350_bat_psy_desc, &psy_cfg);
		if (IS_ERR(battmgr->bat_psy))
			return dev_err_probe(dev, PTR_ERR(battmgr->bat_psy),
					     "failed to register battery power supply\n");

		battmgr->usb_psy = devm_power_supply_register(dev, &sm8350_usb_psy_desc, &psy_cfg_supply);
		if (IS_ERR(battmgr->usb_psy))
			return dev_err_probe(dev, PTR_ERR(battmgr->usb_psy),
					     "failed to register USB power supply\n");

		battmgr->wls_psy = devm_power_supply_register(dev, &sm8350_wls_psy_desc, &psy_cfg_supply);
		if (IS_ERR(battmgr->wls_psy))
			return dev_err_probe(dev, PTR_ERR(battmgr->wls_psy),
					     "failed to register wireless charing power supply\n");
	}

	battmgr->client = devm_pmic_glink_register_client(dev,
							  PMIC_GLINK_OWNER_BATTMGR,
							  qcom_battmgr_callback,
							  qcom_battmgr_pdr_notify,
							  battmgr);
	return PTR_ERR_OR_ZERO(battmgr->client);
}
