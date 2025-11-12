enum dc_status dc_validate_stream(struct dc *dc, struct dc_stream_state *stream)
{
	if (dc == NULL || stream == NULL)
		return DC_ERROR_UNEXPECTED;

	struct dc_link *link = stream->link;
	struct timing_generator *tg = dc->res_pool->timing_generators[0];
	enum dc_status res = DC_OK;

	calculate_phy_pix_clks(stream);

	if (!tg->funcs->validate_timing(tg, &stream->timing))
		res = DC_FAIL_CONTROLLER_VALIDATE;

	if (res == DC_OK) {
		if (link->ep_type == DISPLAY_ENDPOINT_PHY &&
				!link->link_enc->funcs->validate_output_with_stream(
						link->link_enc, stream))
			res = DC_FAIL_ENC_VALIDATE;
	}

	/* TODO: validate audio ASIC caps, encoder */

	if (res == DC_OK)
		res = dc->link_srv->validate_mode_timing(stream,
		      link,
		      &stream->timing);

	return res;
}
