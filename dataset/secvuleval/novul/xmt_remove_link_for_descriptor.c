static void xmt_remove_link_for_descriptor(GF_XMTParser* parser, GF_Descriptor* desc) {

	u32 i=0;
	XMT_ODLink *l, *to_del=NULL;
	while ((l = (XMT_ODLink*)gf_list_enum(parser->od_links, &i)) ) {
		if (l->od && l->od == (GF_ObjectDescriptor*)desc) {
			l->od = NULL;
			to_del = l;
			break;
		}
	}
	if (to_del) {

		i=0;
		GF_Descriptor* subdesc;
		while ((subdesc = gf_list_enum(((GF_ObjectDescriptor*)desc)->ESDescriptors, &i))) {
			if (subdesc) xmt_remove_link_for_descriptor(parser, subdesc);
		}

		gf_list_del_item(parser->od_links, to_del);
		if (to_del->desc_name) gf_free(to_del->desc_name);
		gf_list_del(to_del->mf_urls);
		gf_free(to_del);
	}

	XMT_ESDLink *esdl, *esdl_del=NULL;
	i=0;
	while ((esdl = (XMT_ESDLink *)gf_list_enum(parser->esd_links, &i))) {
		if (esdl->esd && esdl->esd == (GF_ESD*)desc) {
			esdl->esd = NULL;
			esdl_del = esdl;
			break;
		}
	}

	if (esdl_del) {
		gf_list_del_item(parser->esd_links, esdl_del);
		if (esdl_del->desc_name) gf_free(esdl_del->desc_name);
		gf_free(esdl_del);
	}

}