GF_Descriptor *xmt_parse_descriptor(GF_XMTParser *parser, char *name, const GF_XMLAttribute *attributes, u32 nb_attributes, GF_Descriptor *parent)
{
	GF_Err e;
	u32 i;
	u32 fake_desc = 0;
	GF_Descriptor *desc;
	char *xmt_desc_name = NULL, *ocr_ref = NULL, *dep_ref = NULL;
	u32 binaryID = 0;
	u8 tag = gf_odf_get_tag_by_name(name);

	if (!tag) {
		if (!parent) return NULL;
		switch (parent->tag) {
		case GF_ODF_IOD_TAG:
		case GF_ODF_OD_TAG:
			if (!strcmp(name, "Profiles")) fake_desc = 1;
			else if (!strcmp(name, "Descr")) fake_desc = 1;
			else if (!strcmp(name, "esDescr")) fake_desc = 1;
			else if (!strcmp(name, "URL")) fake_desc = 1;
			else return NULL;
			break;
		case GF_ODF_ESD_TAG:
			if (!strcmp(name, "decConfigDescr")) fake_desc = 1;
			else if (!strcmp(name, "slConfigDescr")) fake_desc = 1;
			else return NULL;
			break;
		case GF_ODF_DCD_TAG:
			if (!strcmp(name, "decSpecificInfo")) fake_desc = 1;
			else return NULL;
			break;
		case GF_ODF_SLC_TAG:
			if (!strcmp(name, "custom")) fake_desc = 1;
			else return NULL;
			break;
		case GF_ODF_MUXINFO_TAG:
			if (!strcmp(name, "MP4MuxHints")) fake_desc = 1;
			else return NULL;
			break;
		case GF_ODF_BIFS_CFG_TAG:
			if (!strcmp(name, "commandStream")) fake_desc = 1;
			else if (!strcmp(name, "size")) fake_desc = 2;
			else return NULL;
			break;
		default:
			return NULL;
		}
	}
	if (fake_desc) {
		tag = parent->tag;
		desc = parent;
	} else {
		desc = gf_odf_desc_new(tag);
		if (!desc) return NULL;
	}

	for (i=0; i<nb_attributes; i++) {
		GF_XMLAttribute *att = (GF_XMLAttribute *) &attributes[i];
		if (!att->value || !strlen(att->value)) continue;
		if (!strcmp(att->name, "binaryID")) binaryID = atoi(att->value);
		else if (!stricmp(att->name, "objectDescriptorID")) xmt_desc_name = att->value;
		else if (!strcmp(att->name, "ES_ID")) xmt_desc_name = att->value;
		else if (!strcmp(att->name, "OCR_ES_ID")) ocr_ref = att->value;
		else if (!strcmp(att->name, "dependsOn_ES_ID")) dep_ref = att->value;
		else {
			e = gf_odf_set_field(desc, att->name, att->value);
			if (e) xmt_report(parser, e, "Warning: %s not a valid attribute for descriptor %s", att->name, name);
			//store src path but do not concatenate, othewise we break BT<->XMT conversion ...
			if ((desc->tag==GF_ODF_MUXINFO_TAG) && (!stricmp(att->name, "fileName") || !stricmp(att->name, "url"))) {
				GF_MuxInfo *mux = (GF_MuxInfo *) desc;
				if (!mux->src_url)
					mux->src_url = gf_strdup(parser->load->src_url ? parser->load->src_url : parser->load->fileName);
			}
		}
	}
	if (binaryID || xmt_desc_name) {
		if ((tag == GF_ODF_IOD_TAG) || (tag == GF_ODF_OD_TAG))
			xmt_new_od_link(parser, (GF_ObjectDescriptor *)desc, xmt_desc_name, binaryID);
		else if (tag == GF_ODF_ESD_TAG) {
			xmt_new_esd_link(parser, (GF_ESD *) desc, xmt_desc_name, binaryID);

			/*set references once the esd link has been established*/
			if (ocr_ref) xmt_set_depend_id(parser, (GF_ESD *) desc, ocr_ref, 1);
			if (dep_ref) xmt_set_depend_id(parser, (GF_ESD *) desc, dep_ref, 0);
		}
	}

	if (fake_desc) {
		if (fake_desc==2) {
			GF_BIFSConfig *bcfg = (GF_BIFSConfig *)desc;
			parser->load->ctx->scene_width = bcfg->pixelWidth;
			parser->load->ctx->scene_height = bcfg->pixelHeight;
			parser->load->ctx->is_pixel_metrics = bcfg->pixelMetrics;
		}
		return NULL;
	}
	if (parent) {
		e = gf_odf_desc_add_desc(parent, desc);
		if (e) {
			xmt_report(parser, GF_OK, "Invalid child descriptor");
			xmt_remove_link_for_descriptor(parser, desc);
			gf_odf_desc_del(desc);
			return NULL;
		}
		/*finally check for scene manager streams (scene description, OD, ...)*/
		if (parent->tag == GF_ODF_ESD_TAG) {
			GF_ESD *esd = (GF_ESD *)parent;
			if (esd->decoderConfig) {
				switch (esd->decoderConfig->streamType) {
				case GF_STREAM_SCENE:
				case GF_STREAM_OD:
					if (!esd->decoderConfig->objectTypeIndication) esd->decoderConfig->objectTypeIndication = 1;
					/*watchout for default BIFS stream*/
					if (parser->scene_es && !parser->base_scene_id && (esd->decoderConfig->streamType==GF_STREAM_SCENE)) {
						parser->scene_es->ESID = parser->base_scene_id = esd->ESID;
						parser->scene_es->timeScale = (esd->slConfig && esd->slConfig->timestampResolution) ? esd->slConfig->timestampResolution : 1000;
					} else {
						char *s_name;
						GF_StreamContext *sc = gf_sm_stream_new(parser->load->ctx, esd->ESID, esd->decoderConfig->streamType, esd->decoderConfig->objectTypeIndication);
						/*set default timescale for systems tracks (ignored for other)*/
						if (sc) sc->timeScale = (esd->slConfig && esd->slConfig->timestampResolution) ? esd->slConfig->timestampResolution : 1000;
						if (!parser->base_scene_id && (esd->decoderConfig->streamType==GF_STREAM_SCENE)) parser->base_scene_id = esd->ESID;
						else if (!parser->base_od_id && (esd->decoderConfig->streamType==GF_STREAM_OD)) parser->base_od_id = esd->ESID;

						s_name = xmt_get_es_name(parser, esd->ESID);
						if (sc && s_name && !sc->name) sc->name = gf_strdup(s_name);
					}
					break;
				}
			}
		}
	}
	return desc;
}