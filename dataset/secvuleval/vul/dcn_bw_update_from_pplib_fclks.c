void dcn_bw_update_from_pplib_fclks(
	struct dc *dc,
	struct dm_pp_clock_levels_with_voltage *fclks)
{
	unsigned vmin0p65_idx, vmid0p72_idx, vnom0p8_idx, vmax0p9_idx;

	ASSERT(fclks->num_levels);

	vmin0p65_idx = 0;
	vmid0p72_idx = fclks->num_levels -
		(fclks->num_levels > 2 ? 3 : (fclks->num_levels > 1 ? 2 : 1));
	vnom0p8_idx = fclks->num_levels - (fclks->num_levels > 1 ? 2 : 1);
	vmax0p9_idx = fclks->num_levels - 1;

	dc->dcn_soc->fabric_and_dram_bandwidth_vmin0p65 =
		32 * (fclks->data[vmin0p65_idx].clocks_in_khz / 1000.0) / 1000.0;
	dc->dcn_soc->fabric_and_dram_bandwidth_vmid0p72 =
		dc->dcn_soc->number_of_channels *
		(fclks->data[vmid0p72_idx].clocks_in_khz / 1000.0)
		* ddr4_dram_factor_single_Channel / 1000.0;
	dc->dcn_soc->fabric_and_dram_bandwidth_vnom0p8 =
		dc->dcn_soc->number_of_channels *
		(fclks->data[vnom0p8_idx].clocks_in_khz / 1000.0)
		* ddr4_dram_factor_single_Channel / 1000.0;
	dc->dcn_soc->fabric_and_dram_bandwidth_vmax0p9 =
		dc->dcn_soc->number_of_channels *
		(fclks->data[vmax0p9_idx].clocks_in_khz / 1000.0)
		* ddr4_dram_factor_single_Channel / 1000.0;
}
