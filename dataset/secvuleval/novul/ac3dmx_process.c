GF_Err ac3dmx_process(GF_Filter *filter)
{
	GF_AC3DmxCtx *ctx = gf_filter_get_udta(filter);
	GF_FilterPacket *pck, *dst_pck;
	u8 *output;
	u8 *start;
	u32 pck_size, remain, prev_pck_size;
	u64 cts;

restart:
	cts = GF_FILTER_NO_TS;

	//always reparse duration
	if (!ctx->duration.num)
		ac3dmx_check_dur(filter, ctx);

	if (ctx->opid && !ctx->is_playing)
		return GF_OK;

	pck = gf_filter_pid_get_packet(ctx->ipid);
	if (!pck) {
		if (gf_filter_pid_is_eos(ctx->ipid)) {
			if (!ctx->ac3_buffer_size) {
				if (ctx->opid)
					gf_filter_pid_set_eos(ctx->opid);
				if (ctx->src_pck) gf_filter_pck_unref(ctx->src_pck);
				ctx->src_pck = NULL;
				return GF_EOS;
			}
		} else {
			return GF_OK;
		}
	}

	prev_pck_size = ctx->ac3_buffer_size;
	if (pck && !ctx->resume_from) {
		const u8 *data = gf_filter_pck_get_data(pck, &pck_size);
		if (!pck_size) {
			gf_filter_pid_drop_packet(ctx->ipid);
			return GF_OK;
		}

		//max EAC3 frame is 4096 but we can have side streams, AC3 is 3840 - if we store more than 2 frames consider we have garbage
		if (ctx->ac3_buffer_size>100000) {
			GF_LOG(GF_LOG_WARNING, GF_LOG_MEDIA, ("[AC3Dmx] Trashing %d garbage bytes\n", ctx->ac3_buffer_size));
			ctx->ac3_buffer_size = 0;
		}

		if (ctx->byte_offset != GF_FILTER_NO_BO) {
			u64 byte_offset = gf_filter_pck_get_byte_offset(pck);
			if (!ctx->ac3_buffer_size) {
				ctx->byte_offset = byte_offset;
			} else if (ctx->byte_offset + ctx->ac3_buffer_size != byte_offset) {
				ctx->byte_offset = GF_FILTER_NO_BO;
				if ((byte_offset != GF_FILTER_NO_BO) && (byte_offset>ctx->ac3_buffer_size) ) {
					ctx->byte_offset = byte_offset - ctx->ac3_buffer_size;
				}
			}
		}

		if (ctx->ac3_buffer_size + pck_size > ctx->ac3_buffer_alloc) {
			ctx->ac3_buffer_alloc = ctx->ac3_buffer_size + pck_size;
			ctx->ac3_buffer = gf_realloc(ctx->ac3_buffer, ctx->ac3_buffer_alloc);
		}
		memcpy(ctx->ac3_buffer + ctx->ac3_buffer_size, data, pck_size);
		ctx->ac3_buffer_size += pck_size;
	}

	//input pid sets some timescale - we flushed pending data , update cts
	if (ctx->timescale && pck) {
		cts = gf_filter_pck_get_cts(pck);
		//init cts at first packet
		if (!ctx->cts && (cts != GF_FILTER_NO_TS))
			ctx->cts = cts;
	}

	if (cts == GF_FILTER_NO_TS) {
		//avoids updating cts
		prev_pck_size = 0;
	}

	remain = ctx->ac3_buffer_size;
	start = ctx->ac3_buffer;

	if (ctx->resume_from) {
		start += ctx->resume_from - 1;
		remain -= ctx->resume_from - 1;
		ctx->resume_from = 0;
	}

	if (!ctx->bs) {
		ctx->bs = gf_bs_new(start, remain, GF_BITSTREAM_READ);
	} else {
		gf_bs_reassign_buffer(ctx->bs, start, remain);
	}
	while (remain) {
		u8 *sync;
		Bool res;
		u32 sync_pos, bytes_to_drop=0;

		res = ctx->ac3_parser_bs(ctx->bs, &ctx->hdr, GF_TRUE);

		sync_pos = (u32) gf_bs_get_position(ctx->bs);

		//if not end of stream or no valid frame
		if (pck || !ctx->hdr.framesize) {
			//startcode not found or not enough bytes, gather more
			if (!res || (remain < sync_pos + ctx->hdr.framesize)) {
				if (sync_pos && ctx->hdr.framesize) {
					start += sync_pos;
					remain -= sync_pos;
				}
				break;
			}
			ac3dmx_check_pid(filter, ctx);
		}
		//may happen with very-short streams
		if (!ctx->sample_rate)
			ac3dmx_check_pid(filter, ctx);

		if (!ctx->is_playing) {
			ctx->resume_from = 1 + ctx->ac3_buffer_size - remain;
			return GF_OK;
		}

		if (sync_pos) {
			GF_LOG(GF_LOG_WARNING, GF_LOG_MEDIA, ("[AC3Dmx] %d bytes unrecovered before sync word\n", sync_pos));
		}
		sync = start + sync_pos;

		if (ctx->in_seek) {
			u64 nb_samples_at_seek = (u64) (ctx->start_range * ctx->hdr.sample_rate);
			if (ctx->cts + AC3_FRAME_SIZE >= nb_samples_at_seek) {
				//u32 samples_to_discard = (ctx->cts + ctx->dts_inc) - nb_samples_at_seek;
				ctx->in_seek = GF_FALSE;
			}
		}

		bytes_to_drop = sync_pos + ctx->hdr.framesize;
		if (ctx->timescale && !prev_pck_size &&  (cts != GF_FILTER_NO_TS) ) {
			//trust input CTS if diff is more than one sec
			if ((cts > ctx->cts + ctx->timescale) || (ctx->cts > cts + ctx->timescale))
				ctx->cts = cts;
			cts = GF_FILTER_NO_TS;
		}

		if (!ctx->in_seek && remain >= sync_pos + ctx->hdr.framesize) {
			dst_pck = gf_filter_pck_new_alloc(ctx->opid, ctx->hdr.framesize, &output);
			if (!dst_pck) return GF_OUT_OF_MEM;

			if (ctx->src_pck) gf_filter_pck_merge_properties(ctx->src_pck, dst_pck);
			memcpy(output, sync, ctx->hdr.framesize);
			gf_filter_pck_set_dts(dst_pck, ctx->cts);
			gf_filter_pck_set_cts(dst_pck, ctx->cts);
			if (ctx->timescale && (ctx->timescale!=ctx->sample_rate))
				gf_filter_pck_set_duration(dst_pck, (u32) gf_timestamp_rescale(AC3_FRAME_SIZE, ctx->sample_rate, ctx->timescale));
			else
				gf_filter_pck_set_duration(dst_pck, AC3_FRAME_SIZE);
			gf_filter_pck_set_sap(dst_pck, GF_FILTER_SAP_1);
			gf_filter_pck_set_framing(dst_pck, GF_TRUE, GF_TRUE);

			if (ctx->byte_offset != GF_FILTER_NO_BO) {
				gf_filter_pck_set_byte_offset(dst_pck, ctx->byte_offset + ctx->hdr.framesize);
			}

			gf_filter_pck_send(dst_pck);
		}
		ac3dmx_update_cts(ctx);

		//truncated last frame
		if (bytes_to_drop>remain) {
			GF_LOG(GF_LOG_WARNING, GF_LOG_MEDIA, ("[ADTSDmx] truncated AC3 frame!\n"));
			bytes_to_drop=remain;
		}

		if (!bytes_to_drop) {
			bytes_to_drop = 1;
		}
		start += bytes_to_drop;
		remain -= bytes_to_drop;
		gf_bs_reassign_buffer(ctx->bs, start, remain);

		if (prev_pck_size) {
			if (prev_pck_size > bytes_to_drop) prev_pck_size -= bytes_to_drop;
			else {
				prev_pck_size=0;
				if (ctx->src_pck) gf_filter_pck_unref(ctx->src_pck);
				ctx->src_pck = pck;
				if (pck)
					gf_filter_pck_ref_props(&ctx->src_pck);
			}
		}
		if (ctx->byte_offset != GF_FILTER_NO_BO)
			ctx->byte_offset += bytes_to_drop;
	}

	if (!pck) {
		ctx->ac3_buffer_size = 0;
		//avoid recursive call
		goto restart;
	} else {
		if (remain && (remain < ctx->ac3_buffer_size)) {
			memmove(ctx->ac3_buffer, start, remain);
		}
		ctx->ac3_buffer_size = remain;
		gf_filter_pid_drop_packet(ctx->ipid);
	}
	return GF_OK;
}