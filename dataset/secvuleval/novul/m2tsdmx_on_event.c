
static void m2tsdmx_on_event(GF_M2TS_Demuxer *ts, u32 evt_type, void *param)
{
	u32 i, count;
	GF_Filter *filter = (GF_Filter *) ts->user;
	GF_M2TSDmxCtx *ctx = gf_filter_get_udta(filter);

	switch (evt_type) {
	case GF_M2TS_EVT_PAT_UPDATE:
		break;
	case GF_M2TS_EVT_AIT_FOUND:
		break;
	case GF_M2TS_EVT_PAT_FOUND:
		if (ctx->mux_tune_state==DMX_TUNE_INIT) {
			ctx->mux_tune_state = DMX_TUNE_WAIT_PROGS;
			ctx->wait_for_progs = gf_list_count(ts->programs);
		}
		break;
	case GF_M2TS_EVT_DSMCC_FOUND:
		break;
	case GF_M2TS_EVT_PMT_FOUND:
		m2tsdmx_setup_program(ctx, param);
		if (ctx->mux_tune_state == DMX_TUNE_WAIT_PROGS) {
			gf_assert(ctx->wait_for_progs);
			ctx->wait_for_progs--;
			if (!ctx->wait_for_progs) {
				ctx->mux_tune_state = DMX_TUNE_WAIT_SEEK;
			}
		}
		break;
	case GF_M2TS_EVT_PMT_REPEAT:
		break;
	case GF_M2TS_EVT_PMT_UPDATE:
		m2tsdmx_setup_program(ctx, param);
		break;

	case GF_M2TS_EVT_SDT_FOUND:
	case GF_M2TS_EVT_SDT_UPDATE:
//	case GF_M2TS_EVT_SDT_REPEAT:
		m2tsdmx_update_sdt(ts, NULL);
		break;
	case GF_M2TS_EVT_DVB_GENERAL:
		if (ctx->eit_pid) {
			GF_M2TS_SL_PCK *pck = (GF_M2TS_SL_PCK *)param;
			u8 *data;
			GF_FilterPacket *dst_pck = gf_filter_pck_new_alloc(ctx->eit_pid, pck->data_len, &data);
			if (dst_pck) {
				memcpy(data, pck->data, pck->data_len);
				gf_filter_pck_send(dst_pck);
			}
		}
		break;
	case GF_M2TS_EVT_PES_PCK:
		if (ctx->mux_tune_state) break;
		m2tsdmx_send_packet(ctx, param);
		break;
	case GF_M2TS_EVT_SL_PCK: /* DMB specific */
		if (ctx->mux_tune_state) break;
		m2tsdmx_send_sl_packet(ctx, param);
		break;
	case GF_M2TS_EVT_PES_PCR:
		if (ctx->mux_tune_state) break;
	{
		u64 pcr;
		Bool map_time = GF_FALSE;
		GF_M2TS_PES_PCK *pck = ((GF_M2TS_PES_PCK *) param);
		Bool discontinuity = ( ((GF_M2TS_PES_PCK *) param)->flags & GF_M2TS_PES_PCK_DISCONTINUITY) ? 1 : 0;

		gf_fatal_assert(pck->stream);
		if (!ctx->sigfrag && ctx->index) {
			m2tsdmx_estimate_duration(ctx, (GF_M2TS_ES *) pck->stream);
		}

		if (ctx->map_time_on_prog_id && (ctx->map_time_on_prog_id==pck->stream->program->number)) {
			map_time = GF_TRUE;
		}

		//we forward the PCR on each pid
		pcr = ((GF_M2TS_PES_PCK *) param)->PTS;
		pcr /= 300;
		count = gf_list_count(pck->stream->program->streams);
		for (i=0; i<count; i++) {
			GF_FilterPacket *dst_pck;
			GF_M2TS_PES *stream = gf_list_get(pck->stream->program->streams, i);
			if (!stream->user) continue;

			dst_pck = gf_filter_pck_new_shared(stream->user, NULL, 0, NULL);
			if (!dst_pck) continue;

			gf_filter_pck_set_cts(dst_pck, pcr);
			gf_filter_pck_set_clock_type(dst_pck, discontinuity ? GF_FILTER_CLOCK_PCR_DISC : GF_FILTER_CLOCK_PCR);
			if (pck->stream->is_seg_start) {
				pck->stream->is_seg_start = GF_FALSE;
				gf_filter_pck_set_property(dst_pck, GF_PROP_PCK_CUE_START, &PROP_BOOL(GF_TRUE));
			}
			gf_filter_pck_send(dst_pck);

			if (map_time && (stream->flags & GF_M2TS_ES_IS_PES) ) {
				((GF_M2TS_PES*)stream)->map_pcr = pcr;
			}
		}

		if (map_time) {
			ctx->map_time_on_prog_id = 0;
		}
	}
		break;

	case GF_M2TS_EVT_TDT:
		if (ctx->mux_tune_state) break;
	{
		GF_M2TS_TDT_TOT *tdt = (GF_M2TS_TDT_TOT *)param;
		u64 utc_ts = gf_net_get_utc_ts(tdt->year, tdt->month, tdt->day, tdt->hour, tdt->minute, tdt->second);
		count = gf_list_count(ts->programs );
		for (i=0; i<count; i++) {
			GF_M2TS_Program *prog = gf_list_get(ts->programs, i);
			u32 j, count2 = gf_list_count(prog->streams);
			for (j=0; j<count2; j++) {
				GF_M2TS_ES * stream = gf_list_get(prog->streams, j);
				if (stream->user && (stream->flags & GF_M2TS_ES_IS_PES)) {
					GF_M2TS_PES*pes = (GF_M2TS_PES*)stream;
					pes->map_utc = utc_ts;
					pes->map_utc_pcr = prog->last_pcr_value/300;
				}
			}
			GF_LOG(GF_LOG_DEBUG, GF_LOG_CONTAINER, ("[M2TS In] Mapping TDT Time %04d-%02d-%02dT%02d:%02d:%02d and PCR time "LLD" on program %d\n",
				                                       tdt->year, tdt->month+1, tdt->day, tdt->hour, tdt->minute, tdt->second, prog->last_pcr_value/300, prog->number));
		}
	}
		break;
	case GF_M2TS_EVT_TOT:
		break;

	case GF_M2TS_EVT_DURATION_ESTIMATED:
	{
		u64 duration = ((GF_M2TS_PES_PCK *) param)->PTS;
		count = gf_list_count(ts->programs);
		for (i=0; i<count; i++) {
			GF_M2TS_Program *prog = gf_list_get(ts->programs, i);
			u32 j, count2;
			count2 = gf_list_count(prog->streams);
			for (j=0; j<count2; j++) {
				GF_M2TS_ES * stream = gf_list_get(prog->streams, j);
				if (stream->user) {
					gf_filter_pid_set_property(stream->user, GF_PROP_PID_DURATION, & PROP_FRAC64_INT(duration, 1000) );
				}
			}
		}
	}
	break;

	case GF_M2TS_EVT_TEMI_LOCATION:
	{
		GF_M2TS_TemiLocationDescriptor *temi_l = (GF_M2TS_TemiLocationDescriptor *)param;
		const char *url;
		u32 len;
		GF_BitStream *bs;
		GF_M2TS_ES *es=NULL;
		GF_M2TS_Prop_TEMIInfo *t;
		if ((temi_l->pid<8192) && (ctx->ts->ess[temi_l->pid])) {
			es = ctx->ts->ess[temi_l->pid];
		}
		if (!es || !es->user) {
			GF_LOG(GF_LOG_DEBUG, GF_LOG_CONTAINER, ("[M2TSDmx] TEMI location not assigned to a given PID, not supported\n"));
			break;
		}
		GF_SAFEALLOC(t, GF_M2TS_Prop_TEMIInfo);
		if (!t) break;
		t->timeline_id = temi_l->timeline_id;
		t->is_loc = GF_TRUE;

		bs = gf_bs_new(NULL, 0, GF_BITSTREAM_WRITE);
		if (ctx->temi_url)
			url = ctx->temi_url;
		else
			url = temi_l->external_URL;
		len = url ? (u32) strlen(url) : 0;
		gf_bs_write_data(bs, url, len);
		gf_bs_write_u8(bs, 0);
		gf_bs_write_int(bs, temi_l->is_announce, 1);
		gf_bs_write_int(bs, temi_l->is_splicing, 1);
		gf_bs_write_int(bs, temi_l->reload_external, 1);
		gf_bs_write_int(bs, 0, 5);
		if (temi_l->is_announce) {
			gf_bs_write_u32(bs, temi_l->activation_countdown.den);
			gf_bs_write_u32(bs, temi_l->activation_countdown.num);
		}
		gf_bs_get_content(bs, &t->data, &t->len);
		gf_bs_del(bs);

		if (!es->props) {
			es->props = gf_list_new();
		}
		gf_list_add(es->props, t);
	}
	break;
	case GF_M2TS_EVT_TEMI_TIMECODE:
	{
		GF_M2TS_TemiTimecodeDescriptor *temi_t = (GF_M2TS_TemiTimecodeDescriptor*)param;
		GF_BitStream *bs;
		GF_M2TS_Prop_TEMIInfo *t;
		GF_M2TS_ES *es=NULL;
		if ((temi_t->pid<8192) && (ctx->ts->ess[temi_t->pid])) {
			es = ctx->ts->ess[temi_t->pid];
		}
		if (!es || !es->user) {
			GF_LOG(GF_LOG_DEBUG, GF_LOG_CONTAINER, ("[M2TSDmx] TEMI timing not assigned to a given PID, not supported\n"));
			break;
		}
		GF_SAFEALLOC(t, GF_M2TS_Prop_TEMIInfo);
		if (!t) break;
		t->type = M2TS_TEMI_INFO;
		t->timeline_id = temi_t->timeline_id;

		bs = gf_bs_new(NULL, 0, GF_BITSTREAM_WRITE);
		gf_bs_write_u32(bs, temi_t->media_timescale);
		gf_bs_write_u64(bs, temi_t->media_timestamp);
		gf_bs_write_u64(bs, temi_t->pes_pts);
		gf_bs_write_int(bs, temi_t->force_reload, 1);
		gf_bs_write_int(bs, temi_t->is_paused, 1);
		gf_bs_write_int(bs, temi_t->is_discontinuity, 1);
		gf_bs_write_int(bs, temi_t->ntp ? 1 : 0, 1);
		gf_bs_write_int(bs, 0, 4);
		if (temi_t->ntp)
			gf_bs_write_u64(bs, temi_t->ntp);

		gf_bs_get_content(bs, &t->data, &t->len);
		gf_bs_del(bs);

		if (!es->props) {
			es->props = gf_list_new();
		}
		gf_list_add(es->props, t);
	}
	break;
	case GF_M2TS_EVT_ID3:
	{
		GF_M2TS_PES_PCK *pck = (GF_M2TS_PES_PCK*)param;
		GF_BitStream *bs;
		GF_M2TS_Prop *t;
		u32 count = gf_list_count(pck->stream->program->streams);
		for (i=0; i<count; i++) {
			GF_M2TS_PES *es = gf_list_get(pck->stream->program->streams, i);
			if (!es->user) {
				GF_LOG(GF_LOG_DEBUG, GF_LOG_CONTAINER, ("[M2TSDmx] ID3 metadata not assigned to a given PID, not supported\n"));
				continue;
			}

			// attach ID3 markers to audio
			GF_FilterPid *opid = (GF_FilterPid *)es->user;
			const GF_PropertyValue *p = gf_filter_pid_get_property(opid, GF_PROP_PID_STREAM_TYPE);
			if (!p) return;
			if (p->value.uint != GF_STREAM_AUDIO)
				continue;

			GF_SAFEALLOC(t, GF_M2TS_Prop);
			if (!t) break;
			t->type = M2TS_ID3;
			bs = gf_bs_new(NULL, 0, GF_BITSTREAM_WRITE);
			gf_bs_write_u32(bs, 90000);                     // timescale
			gf_bs_write_u64(bs, pck->PTS);                  // pts
			gf_bs_write_u32(bs, pck->data_len);				// data length (bytes)
			gf_bs_write_data(bs, pck->data, pck->data_len); // data
			gf_bs_get_content(bs, &t->data, &t->len);
			gf_bs_del(bs);

			if (!es->props) {
				es->props = gf_list_new();
			}
			gf_list_add(es->props, t);
		}
	}
	break;
	case GF_M2TS_EVT_SCTE35_SPLICE_INFO:
	{
		GF_M2TS_SL_PCK *pck = (GF_M2TS_SL_PCK*)param;
		GF_BitStream *bs;
		GF_M2TS_Prop *t;

		//for now all SCTE35 must be associated with a stream
		if (!pck->stream) return;

		// convey SCTE35 splice info to all streams of the program
		u32 count = gf_list_count(pck->stream->program->streams);
		for (i=0; i<count; i++) {
			GF_M2TS_PES *es = gf_list_get(pck->stream->program->streams, i);
			if (!es->user) {
				GF_LOG(GF_LOG_DEBUG, GF_LOG_CONTAINER, ("[M2TSDmx] SCTE35 section not assigned to a given PID, not supported\n"));
				continue;
			}

			// attach SCTE35 info to video only
			GF_FilterPid *opid = (GF_FilterPid *)es->user;
			const GF_PropertyValue *p = gf_filter_pid_get_property(opid, GF_PROP_PID_STREAM_TYPE);
			if (!p) return;
			if (p->value.uint != GF_STREAM_VISUAL)
				continue;

			GF_SAFEALLOC(t, GF_M2TS_Prop);
			if (!t) break;
			t->type = M2TS_SCTE35;
			bs = gf_bs_new(NULL, 0, GF_BITSTREAM_WRITE);
			// ANSI/SCTE 67 2017 (13.1.1.3): "the entire SCTE 35 splice_info_section starting at the table_id and ending with the CRC_32"
			gf_bs_write_data(bs, pck->data, pck->data_len);
			gf_bs_get_content(bs, &t->data, &t->len);
			gf_bs_del(bs);

			if (!es->props) {
				es->props = gf_list_new();
			}
			gf_list_add(es->props, t);

			// send SCTE35 info only to the first video pid
			break;
		}
	}
	break;
	case GF_M2TS_EVT_STREAM_REMOVED:
	{
		GF_M2TS_ES *es = (GF_M2TS_ES *)param;
		if (es && es->props) {
			while (gf_list_count(es->props)) {
				GF_M2TS_Prop *t = gf_list_pop_back(es->props);
				gf_free(t->data);
				gf_free(t);
			}
			gf_list_del(es->props);
		}
	}
		break;
	}