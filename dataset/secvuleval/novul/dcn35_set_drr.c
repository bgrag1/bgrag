void dcn35_set_drr(struct pipe_ctx **pipe_ctx,
		int num_pipes, struct dc_crtc_timing_adjust adjust)
{
	int i = 0;
	struct drr_params params = {0};
	// DRR set trigger event mapped to OTG_TRIG_A
	unsigned int event_triggers = 0x2;//Bit[1]: OTG_TRIG_A
	// Note DRR trigger events are generated regardless of whether num frames met.
	unsigned int num_frames = 2;

	params.vertical_total_max = adjust.v_total_max;
	params.vertical_total_min = adjust.v_total_min;
	params.vertical_total_mid = adjust.v_total_mid;
	params.vertical_total_mid_frame_num = adjust.v_total_mid_frame_num;

	for (i = 0; i < num_pipes; i++) {
		/* dc_state_destruct() might null the stream resources, so fetch tg
		 * here first to avoid a race condition. The lifetime of the pointee
		 * itself (the timing_generator object) is not a problem here.
		 */
		struct timing_generator *tg = pipe_ctx[i]->stream_res.tg;

		if ((tg != NULL) && tg->funcs) {
			struct dc_crtc_timing *timing = &pipe_ctx[i]->stream->timing;
			struct dc *dc = pipe_ctx[i]->stream->ctx->dc;

			if (dc->debug.static_screen_wait_frames) {
				unsigned int frame_rate = timing->pix_clk_100hz / (timing->h_total * timing->v_total);

				if (frame_rate >= 120 && dc->caps.ips_support &&
					dc->config.disable_ips != DMUB_IPS_DISABLE_ALL) {
					/*ips enable case*/
					num_frames = 2 * (frame_rate % 60);
				}
			}
			if (tg->funcs->set_drr)
				tg->funcs->set_drr(tg, &params);
			if (adjust.v_total_max != 0 && adjust.v_total_min != 0)
				if (tg->funcs->set_static_screen_control)
					tg->funcs->set_static_screen_control(
						tg, event_triggers, num_frames);
		}
	}
}
