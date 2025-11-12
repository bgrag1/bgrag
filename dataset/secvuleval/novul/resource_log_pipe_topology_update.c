void resource_log_pipe_topology_update(struct dc *dc, struct dc_state *state)
{
	struct pipe_ctx *otg_master;
	int stream_idx, phantom_stream_idx;
	DC_LOGGER_INIT(dc->ctx->logger);

	DC_LOG_DC("    pipe topology update");
	DC_LOG_DC("  ________________________");
	for (stream_idx = 0; stream_idx < state->stream_count; stream_idx++) {
		if (state->streams[stream_idx]->is_phantom)
			continue;

		otg_master = resource_get_otg_master_for_stream(
				&state->res_ctx, state->streams[stream_idx]);

		if (!otg_master)
			continue;

		resource_log_pipe_for_stream(dc, state, otg_master, stream_idx);
	}
	if (state->phantom_stream_count > 0) {
		DC_LOG_DC(" |    (phantom pipes)     |");
		for (stream_idx = 0; stream_idx < state->stream_count; stream_idx++) {
			if (state->stream_status[stream_idx].mall_stream_config.type != SUBVP_MAIN)
				continue;

			phantom_stream_idx = resource_stream_to_stream_idx(state,
					state->stream_status[stream_idx].mall_stream_config.paired_stream);
			otg_master = resource_get_otg_master_for_stream(
					&state->res_ctx, state->streams[phantom_stream_idx]);
			if (!otg_master)
				continue;

			resource_log_pipe_for_stream(dc, state, otg_master, stream_idx);
		}
	}
	DC_LOG_DC(" |________________________|\n");
}
