void dcn10_set_drr(struct pipe_ctx **pipe_ctx,
		int num_pipes, struct dc_crtc_timing_adjust adjust)
{
	int i = 0;
	struct drr_params params = {0};
	// DRR set trigger event mapped to OTG_TRIG_A (bit 11) for manual control flow
	unsigned int event_triggers = 0x800;
	// Note DRR trigger events are generated regardless of whether num frames met.
	unsigned int num_frames = 2;

	params.vertical_total_max = adjust.v_total_max;
	params.vertical_total_min = adjust.v_total_min;
	params.vertical_total_mid = adjust.v_total_mid;
	params.vertical_total_mid_frame_num = adjust.v_total_mid_frame_num;
	/* TODO: If multiple pipes are to be supported, you need
	 * some GSL stuff. Static screen triggers may be programmed differently
	 * as well.
	 */
	for (i = 0; i < num_pipes; i++) {
		if ((pipe_ctx[i]->stream_res.tg != NULL) && pipe_ctx[i]->stream_res.tg->funcs) {
			if (pipe_ctx[i]->stream_res.tg->funcs->set_drr)
				pipe_ctx[i]->stream_res.tg->funcs->set_drr(
					pipe_ctx[i]->stream_res.tg, &params);
			if (adjust.v_total_max != 0 && adjust.v_total_min != 0)
				if (pipe_ctx[i]->stream_res.tg->funcs->set_static_screen_control)
					pipe_ctx[i]->stream_res.tg->funcs->set_static_screen_control(
						pipe_ctx[i]->stream_res.tg,
						event_triggers, num_frames);
		}
	}
}
