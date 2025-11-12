bool dcn20_get_dcc_compression_cap(const struct dc *dc,
		const struct dc_dcc_surface_param *input,
		struct dc_surface_dcc_cap *output)
{
	if (dc->res_pool->hubbub->funcs->get_dcc_compression_cap)
		return dc->res_pool->hubbub->funcs->get_dcc_compression_cap(
			dc->res_pool->hubbub, input, output);

	return false;
}
