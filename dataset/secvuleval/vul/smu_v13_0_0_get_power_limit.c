static int smu_v13_0_0_get_power_limit(struct smu_context *smu,
						uint32_t *current_power_limit,
						uint32_t *default_power_limit,
						uint32_t *max_power_limit,
						uint32_t *min_power_limit)
{
	struct smu_table_context *table_context = &smu->smu_table;
	struct smu_13_0_0_powerplay_table *powerplay_table =
		(struct smu_13_0_0_powerplay_table *)table_context->power_play_table;
	PPTable_t *pptable = table_context->driver_pptable;
	SkuTable_t *skutable = &pptable->SkuTable;
	uint32_t power_limit, od_percent_upper, od_percent_lower;
	uint32_t msg_limit = skutable->MsgLimits.Power[PPT_THROTTLER_PPT0][POWER_SOURCE_AC];

	if (smu_v13_0_get_current_power_limit(smu, &power_limit))
		power_limit = smu->adev->pm.ac_power ?
			      skutable->SocketPowerLimitAc[PPT_THROTTLER_PPT0] :
			      skutable->SocketPowerLimitDc[PPT_THROTTLER_PPT0];

	if (current_power_limit)
		*current_power_limit = power_limit;
	if (default_power_limit)
		*default_power_limit = power_limit;

	if (smu->od_enabled)
		od_percent_upper = le32_to_cpu(powerplay_table->overdrive_table.max[SMU_13_0_0_ODSETTING_POWERPERCENTAGE]);
	else
		od_percent_upper = 0;

	od_percent_lower = le32_to_cpu(powerplay_table->overdrive_table.min[SMU_13_0_0_ODSETTING_POWERPERCENTAGE]);

	dev_dbg(smu->adev->dev, "od percent upper:%d, od percent lower:%d (default power: %d)\n",
					od_percent_upper, od_percent_lower, power_limit);

	if (max_power_limit) {
		*max_power_limit = msg_limit * (100 + od_percent_upper);
		*max_power_limit /= 100;
	}

	if (min_power_limit) {
		*min_power_limit = power_limit * (100 - od_percent_lower);
		*min_power_limit /= 100;
	}

	return 0;
}
