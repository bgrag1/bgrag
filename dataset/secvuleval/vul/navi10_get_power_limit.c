static int navi10_get_power_limit(struct smu_context *smu,
					uint32_t *current_power_limit,
					uint32_t *default_power_limit,
					uint32_t *max_power_limit,
					uint32_t *min_power_limit)
{
	struct smu_11_0_powerplay_table *powerplay_table =
		(struct smu_11_0_powerplay_table *)smu->smu_table.power_play_table;
	struct smu_11_0_overdrive_table *od_settings = smu->od_settings;
	PPTable_t *pptable = smu->smu_table.driver_pptable;
	uint32_t power_limit, od_percent_upper, od_percent_lower;

	if (smu_v11_0_get_current_power_limit(smu, &power_limit)) {
		/* the last hope to figure out the ppt limit */
		if (!pptable) {
			dev_err(smu->adev->dev, "Cannot get PPT limit due to pptable missing!");
			return -EINVAL;
		}
		power_limit =
			pptable->SocketPowerLimitAc[PPT_THROTTLER_PPT0];
	}

	if (current_power_limit)
		*current_power_limit = power_limit;
	if (default_power_limit)
		*default_power_limit = power_limit;

	if (smu->od_enabled &&
		    navi10_od_feature_is_supported(od_settings, SMU_11_0_ODCAP_POWER_LIMIT))
		od_percent_upper = le32_to_cpu(powerplay_table->overdrive_table.max[SMU_11_0_ODSETTING_POWERPERCENTAGE]);
	else
		od_percent_upper = 0;

	od_percent_lower = le32_to_cpu(powerplay_table->overdrive_table.min[SMU_11_0_ODSETTING_POWERPERCENTAGE]);

	dev_dbg(smu->adev->dev, "od percent upper:%d, od percent lower:%d (default power: %d)\n",
					od_percent_upper, od_percent_lower, power_limit);

	if (max_power_limit) {
		*max_power_limit = power_limit * (100 + od_percent_upper);
		*max_power_limit /= 100;
	}

	if (min_power_limit) {
		*min_power_limit = power_limit * (100 - od_percent_lower);
		*min_power_limit /= 100;
	}

	return 0;
}
