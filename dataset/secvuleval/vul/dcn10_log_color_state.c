static void dcn10_log_color_state(struct dc *dc,
				  struct dc_log_buffer_ctx *log_ctx)
{
	struct dc_context *dc_ctx = dc->ctx;
	struct resource_pool *pool = dc->res_pool;
	int i;

	DTN_INFO("DPP:    IGAM format    IGAM mode    DGAM mode    RGAM mode"
		 "  GAMUT adjust  "
		 "C11        C12        C13        C14        "
		 "C21        C22        C23        C24        "
		 "C31        C32        C33        C34        \n");
	for (i = 0; i < pool->pipe_count; i++) {
		struct dpp *dpp = pool->dpps[i];
		struct dcn_dpp_state s = {0};

		dpp->funcs->dpp_read_state(dpp, &s);
		dpp->funcs->dpp_get_gamut_remap(dpp, &s.gamut_remap);

		if (!s.is_enabled)
			continue;

		DTN_INFO("[%2d]:  %11xh  %11s    %9s    %9s"
			 "  %12s  "
			 "%010lld %010lld %010lld %010lld "
			 "%010lld %010lld %010lld %010lld "
			 "%010lld %010lld %010lld %010lld",
				dpp->inst,
				s.igam_input_format,
				(s.igam_lut_mode == 0) ? "BypassFixed" :
					((s.igam_lut_mode == 1) ? "BypassFloat" :
					((s.igam_lut_mode == 2) ? "RAM" :
					((s.igam_lut_mode == 3) ? "RAM" :
								 "Unknown"))),
				(s.dgam_lut_mode == 0) ? "Bypass" :
					((s.dgam_lut_mode == 1) ? "sRGB" :
					((s.dgam_lut_mode == 2) ? "Ycc" :
					((s.dgam_lut_mode == 3) ? "RAM" :
					((s.dgam_lut_mode == 4) ? "RAM" :
								 "Unknown")))),
				(s.rgam_lut_mode == 0) ? "Bypass" :
					((s.rgam_lut_mode == 1) ? "sRGB" :
					((s.rgam_lut_mode == 2) ? "Ycc" :
					((s.rgam_lut_mode == 3) ? "RAM" :
					((s.rgam_lut_mode == 4) ? "RAM" :
								 "Unknown")))),
				(s.gamut_remap.gamut_adjust_type == 0) ? "Bypass" :
					((s.gamut_remap.gamut_adjust_type == 1) ? "HW" :
										  "SW"),
				s.gamut_remap.temperature_matrix[0].value,
				s.gamut_remap.temperature_matrix[1].value,
				s.gamut_remap.temperature_matrix[2].value,
				s.gamut_remap.temperature_matrix[3].value,
				s.gamut_remap.temperature_matrix[4].value,
				s.gamut_remap.temperature_matrix[5].value,
				s.gamut_remap.temperature_matrix[6].value,
				s.gamut_remap.temperature_matrix[7].value,
				s.gamut_remap.temperature_matrix[8].value,
				s.gamut_remap.temperature_matrix[9].value,
				s.gamut_remap.temperature_matrix[10].value,
				s.gamut_remap.temperature_matrix[11].value);
		DTN_INFO("\n");
	}
	DTN_INFO("\n");
	DTN_INFO("DPP Color Caps: input_lut_shared:%d  icsc:%d"
		 "  dgam_ram:%d  dgam_rom: srgb:%d,bt2020:%d,gamma2_2:%d,pq:%d,hlg:%d"
		 "  post_csc:%d  gamcor:%d  dgam_rom_for_yuv:%d  3d_lut:%d"
		 "  blnd_lut:%d  oscs:%d\n\n",
		 dc->caps.color.dpp.input_lut_shared,
		 dc->caps.color.dpp.icsc,
		 dc->caps.color.dpp.dgam_ram,
		 dc->caps.color.dpp.dgam_rom_caps.srgb,
		 dc->caps.color.dpp.dgam_rom_caps.bt2020,
		 dc->caps.color.dpp.dgam_rom_caps.gamma2_2,
		 dc->caps.color.dpp.dgam_rom_caps.pq,
		 dc->caps.color.dpp.dgam_rom_caps.hlg,
		 dc->caps.color.dpp.post_csc,
		 dc->caps.color.dpp.gamma_corr,
		 dc->caps.color.dpp.dgam_rom_for_yuv,
		 dc->caps.color.dpp.hw_3d_lut,
		 dc->caps.color.dpp.ogam_ram,
		 dc->caps.color.dpp.ocsc);

	DTN_INFO("MPCC:  OPP  DPP  MPCCBOT  MODE  ALPHA_MODE  PREMULT  OVERLAP_ONLY  IDLE\n");
	for (i = 0; i < pool->mpcc_count; i++) {
		struct mpcc_state s = {0};

		pool->mpc->funcs->read_mpcc_state(pool->mpc, i, &s);
		if (s.opp_id != 0xf)
			DTN_INFO("[%2d]:  %2xh  %2xh  %6xh  %4d  %10d  %7d  %12d  %4d\n",
				i, s.opp_id, s.dpp_id, s.bot_mpcc_id,
				s.mode, s.alpha_mode, s.pre_multiplied_alpha, s.overlap_only,
				s.idle);
	}
	DTN_INFO("\n");
	DTN_INFO("MPC Color Caps: gamut_remap:%d, 3dlut:%d, ogam_ram:%d, ocsc:%d\n\n",
		 dc->caps.color.mpc.gamut_remap,
		 dc->caps.color.mpc.num_3dluts,
		 dc->caps.color.mpc.ogam_ram,
		 dc->caps.color.mpc.ocsc);
}
