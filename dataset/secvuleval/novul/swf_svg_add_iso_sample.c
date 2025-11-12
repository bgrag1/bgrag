
static GF_Err swf_svg_add_iso_sample(void *user, const u8 *data, u32 length, u64 timestamp, Bool isRap)
{
	GF_FilterPacket *pck;
	u8 *pck_data;
	GF_TXTIn *ctx = (GF_TXTIn *)user;
	if (!data || !length) return GF_BAD_PARAM;

	if (ctx->seek_state==2) {
		Double ts = (Double) timestamp;
		ts/=1000;
		if (ts<ctx->start_range) return GF_OK;
		ctx->seek_state = 0;
	}

	pck = gf_filter_pck_new_alloc(ctx->opid, length, &pck_data);
	if (pck) {
		memcpy(pck_data, data, length);
		gf_filter_pck_set_cts(pck, (u64) (ctx->timescale*timestamp/1000) );
		gf_filter_pck_set_sap(pck, isRap ? GF_FILTER_SAP_1 : GF_FILTER_SAP_NONE);
		gf_filter_pck_set_framing(pck, GF_TRUE, GF_FALSE);

		gf_filter_pck_send(pck);
	}

	if (txtin_check_blocking(ctx))
		ctx->is_suspended = GF_TRUE;
	return GF_OK;