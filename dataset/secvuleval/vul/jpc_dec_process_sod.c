static int jpc_dec_process_sod(jpc_dec_t *dec, jpc_ms_t *ms)
{
	jpc_dec_tile_t *tile;
	int pos;

	JAS_UNUSED(ms);

	if (!(tile = dec->curtile)) {
		return -1;
	}

	if (!tile->partno) {
		if (!jpc_dec_cp_isvalid(tile->cp)) {
			return -1;
		}
		if (jpc_dec_cp_prepare(tile->cp)) {
			return -1;
		}
		if (jpc_dec_tileinit(dec, tile)) {
			return -1;
		}
	}

	/* Are packet headers stored in the main header or tile-part header? */
	if (dec->pkthdrstreams) {
		/* Get the stream containing the packet header data for this
		  tile-part. */
		if (!(tile->pkthdrstream = jpc_streamlist_remove(dec->pkthdrstreams, 0))) {
			return -1;
		}
	}

	if (tile->pptstab) {
		if (!tile->pkthdrstream) {
			if (!(tile->pkthdrstream = jas_stream_memopen(0, 0))) {
				return -1;
			}
		}
		pos = jas_stream_tell(tile->pkthdrstream);
		jas_stream_seek(tile->pkthdrstream, 0, SEEK_END);
		if (jpc_pptstabwrite(tile->pkthdrstream, tile->pptstab)) {
			return -1;
		}
		jas_stream_seek(tile->pkthdrstream, pos, SEEK_SET);
		jpc_ppxstab_destroy(tile->pptstab);
		tile->pptstab = 0;
	}

	if (jas_get_debug_level() >= 10) {
		jpc_dec_dump(dec);
	}

	if (jpc_dec_decodepkts(dec, (tile->pkthdrstream) ? tile->pkthdrstream :
	  dec->in, dec->in)) {
		jas_logerrorf("jpc_dec_decodepkts failed\n");
		return -1;
	}

	/* Gobble any unconsumed tile data. */
	if (dec->curtileendoff > 0) {
		long curoff;
		uint_fast32_t n;
		curoff = jas_stream_getrwcount(dec->in);
		if (curoff < dec->curtileendoff) {
			n = dec->curtileendoff - curoff;
			jas_logwarnf("warning: ignoring trailing garbage (%lu bytes)\n",
			  (unsigned long) n);

			while (n-- > 0) {
				if (jas_stream_getc(dec->in) == EOF) {
					jas_logerrorf("read error\n");
					return -1;
				}
			}
		} else if (curoff > dec->curtileendoff) {
			jas_logwarnf("warning: not enough tile data (%lu bytes)\n",
			  (unsigned long) curoff - dec->curtileendoff);
		}

	}

	if (tile->numparts > 0 && tile->partno == tile->numparts - 1) {
		if (jpc_dec_tiledecode(dec, tile)) {
			return -1;
		}
		jpc_dec_tilefini(dec, tile);
	}

	dec->curtile = 0;

	/* Increment the expected tile-part number. */
	++tile->partno;

	/* We should expect to encounter a SOT marker segment next. */
	dec->state = JPC_TPHSOT;

	return 0;
}