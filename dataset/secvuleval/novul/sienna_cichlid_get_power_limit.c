static int sienna_cichlid_get_power_limit(struct smu_context *smu,
					  uint32_t *current_power_limit,
					  uint32_t *default_power_limit,
					  uint32_t *max_power_limit,
					  uint32_t *min_power_limit)
{
	struct smu_11_0_7_powerplay_table *powerplay_table =
		(struct smu_11_0_7_powerplay_table *)smu->smu_table.power_play_table;
	uint32_t power_limit, od_percent_upper = 0, od_percent_lower = 0;
	uint16_t *table_member;

	GET_PPTABLE_MEMBER(SocketPowerLimitAc, &table_member);

	if (smu_v11_0_get_current_power_limit(smu, &power_limit)) {
		power_limit =
			table_member[PPT_THROTTLER_PPT0];
	}

	if (current_power_limit)
		*current_power_limit = power_limit;
	if (default_power_limit)
		*default_power_limit = power_limit;

	if (powerplay_table) {
		if (smu->od_enabled)
			od_percent_upper = le32_to_cpu(powerplay_table->overdrive_table.max[SMU_11_0_7_ODSETTING_POWERPERCENTAGE]);
		else
			od_percent_upper = 0;

		od_percent_lower = le32_to_cpu(powerplay_table->overdrive_table.min[SMU_11_0_7_ODSETTING_POWERPERCENTAGE]);
	}

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
