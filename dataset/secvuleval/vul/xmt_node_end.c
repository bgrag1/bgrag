static void xmt_node_end(void *sax_cbck, const char *name, const char *name_space)
{
	u32 tag;
	GF_XMTParser *parser = (GF_XMTParser *)sax_cbck;
	XMTNodeStack *top;
	GF_Descriptor *desc;
	GF_Node *node = NULL;
	if (!parser->doc_type || !parser->state) return;

	top = (XMTNodeStack *)gf_list_last(parser->nodes);

	if (!top) {
		/*check descr*/
		desc = (GF_Descriptor*)gf_list_last(parser->descriptors);
		if (desc && (desc->tag == gf_odf_get_tag_by_name((char *)name)) ) {

			/*assign timescales once the ESD has been parsed*/
			if (desc->tag == GF_ODF_ESD_TAG) {
				GF_ESD *esd = (GF_ESD*)desc;
				GF_StreamContext *sc = gf_sm_stream_new(parser->load->ctx, esd->ESID, esd->decoderConfig ? esd->decoderConfig->streamType : 0, esd->decoderConfig ? esd->decoderConfig->objectTypeIndication : 0);
				if (sc && esd->slConfig && esd->slConfig->timestampResolution)
					sc->timeScale = esd->slConfig->timestampResolution;
			}

			gf_list_rem_last(parser->descriptors);
			if (gf_list_count(parser->descriptors)) return;

			if ((parser->doc_type==1) && (parser->state==XMT_STATE_HEAD) && parser->load->ctx && !parser->load->ctx->root_od) {
				parser->load->ctx->root_od = (GF_ObjectDescriptor *)desc;
			}
			else if (!parser->od_command) {
				xmt_report(parser, GF_OK, "Warning: descriptor %s defined outside scene scope - skipping", name);
				gf_odf_desc_del(desc);
				XMT_ODLink *prev_l = gf_list_last(parser->od_links);
				if ((GF_Descriptor *) prev_l->od == desc) {
					gf_list_pop_back(parser->od_links);
					if (prev_l->desc_name) gf_free(prev_l->desc_name);
					gf_list_del(prev_l->mf_urls);
					gf_free(prev_l);
				}
			} else {
				switch (parser->od_command->tag) {
				case GF_ODF_ESD_UPDATE_TAG:
					gf_list_add( ((GF_ESDUpdate *)parser->od_command)->ESDescriptors, desc);
					break;
				/*same struct for OD update and IPMP update*/
				case GF_ODF_OD_UPDATE_TAG:
				case GF_ODF_IPMP_UPDATE_TAG:
					gf_list_add( ((GF_ODUpdate *)parser->od_command)->objectDescriptors, desc);
					break;
				}
			}

			return;
		}
		if (parser->state == XMT_STATE_HEAD) {
			if ((parser->doc_type == 1) && !strcmp(name, "Header")) parser->state = XMT_STATE_BODY;
			/*X3D header*/
			else if ((parser->doc_type == 2) && !strcmp(name, "head")) {
				parser->state = XMT_STATE_BODY;
				/*create a group at root level*/
				tag = xmt_get_node_tag(parser, "Group");
				node = gf_node_new(parser->load->scene_graph, tag);
				gf_node_register(node, NULL);
				gf_sg_set_root_node(parser->load->scene_graph, node);
				gf_node_init(node);

				/*create a default top for X3D*/
				GF_SAFEALLOC(parser->x3d_root, XMTNodeStack);
				if (!parser->x3d_root) {
					GF_LOG(GF_LOG_ERROR, GF_LOG_PARSER, ("Failed to allocate X3D root\n"));
					return;
				}
				parser->x3d_root->node = node;
			}
			/*XMT-O header*/
			else if ((parser->doc_type == 3) && !strcmp(name, "head")) parser->state = XMT_STATE_BODY;
		}
		else if (parser->state == XMT_STATE_ELEMENTS) {
			gf_assert((parser->doc_type != 1) || parser->command);
			if (!strcmp(name, "Replace") || !strcmp(name, "Insert") || !strcmp(name, "Delete")) {
				parser->command = NULL;
				parser->state = XMT_STATE_COMMANDS;
			}
			else if (!strcmp(name, "repField")) {
				parser->state = XMT_STATE_COMMANDS;
			}
			/*end proto*/
			else if (!strcmp(name, "ProtoDeclare") || !strcmp(name, "ExternProtoDeclare"))  {
				GF_Proto *cur = parser->parsing_proto;
				xmt_resolve_routes(parser);
				parser->parsing_proto = (GF_Proto*)cur->userpriv;
				parser->load->scene_graph = cur->parent_graph;
				cur->userpriv = NULL;
			}
			else if (parser->proto_field && !strcmp(name, "field")) parser->proto_field = NULL;
			/*end X3D body*/
			else if ((parser->doc_type == 2) && !strcmp(name, "Scene")) parser->state = XMT_STATE_BODY_END;
		}
		else if (parser->state == XMT_STATE_COMMANDS) {
			/*end XMT-A body*/
			if ((parser->doc_type == 1) && !strcmp(name, "Body")) parser->state = XMT_STATE_BODY_END;
			/*end X3D body*/
			else if ((parser->doc_type == 2) && !strcmp(name, "Scene")) parser->state = XMT_STATE_BODY_END;
			/*end XMT-O body*/
			else if ((parser->doc_type == 3) && !strcmp(name, "body")) parser->state = XMT_STATE_BODY_END;

			/*end scene command*/
			else if (!strcmp(name, "Replace") || !strcmp(name, "Insert") || !strcmp(name, "Delete") )  {
				/*restore parent command if in CommandBuffer*/
				if (parser->command && parser->command_buffer && parser->command_buffer->buffer) {
					//empty <Insert>
					if ((parser->command->tag==GF_SG_ROUTE_INSERT) && !parser->command->fromNodeID) {
						gf_list_del_item(parser->command_buffer->commandList, parser->command);
					}

					parser->command = (GF_Command*) parser->command_buffer->buffer;
					parser->command_buffer->buffer = NULL;
					parser->command_buffer = NULL;
				} else {
					//empty <Insert>
					if (parser->command && (parser->command->tag==GF_SG_ROUTE_INSERT) && !parser->command->fromNodeID) {
						gf_list_del_item(parser->scene_au->commands, parser->command);
					}
					parser->command = NULL;
				}
			}
			/*end OD command*/
			else if (!strcmp(name, "ObjectDescriptorUpdate") || !strcmp(name, "ObjectDescriptorRemove")
			         || !strcmp(name, "ES_DescriptorUpdate") || !strcmp(name, "ES_DescriptorRemove")
			         || !strcmp(name, "IPMP_DescriptorUpdate") || !strcmp(name, "IPMP_DescriptorRemove") ) {
				parser->od_command = NULL;
			}

			else if (!strcmp(name, "par"))
				parser->in_com = 1;


		}
		else if (parser->state == XMT_STATE_BODY_END) {
			/*end XMT-A*/
			if ((parser->doc_type == 1) && !strcmp(name, "XMT-A")) parser->state = XMT_STATE_END;
			/*end X3D*/
			else if ((parser->doc_type == 2) && !strcmp(name, "X3D")) {
				while (1) {
					GF_Node *n = (GF_Node *)gf_list_last(parser->script_to_load);
					if (!n) break;
					gf_list_rem_last(parser->script_to_load);
					gf_sg_script_load(n);
				}
				gf_list_del(parser->script_to_load);
				parser->script_to_load = NULL;
				parser->state = XMT_STATE_END;
			}
			/*end XMT-O*/
			else if ((parser->doc_type == 3) && !strcmp(name, "XMT-O")) parser->state = XMT_STATE_END;
		}
		return;
	}
	/*only remove created nodes ... */
	tag = xmt_get_node_tag(parser, name);
	if (!tag) {
		if (top->container_field.name) {
			if (!strcmp(name, top->container_field.name)) {
				if (top->container_field.fieldType==GF_SG_VRML_SFCOMMANDBUFFER) {
					parser->state = XMT_STATE_ELEMENTS;
					parser->command = (GF_Command *) (void *) parser->command_buffer->buffer;
					parser->command_buffer->buffer = NULL;
					parser->command_buffer = NULL;
				}
				top->container_field.far_ptr = NULL;
				top->container_field.name = NULL;
				top->last = NULL;
			}
			/*end of command inside an command (conditional.buffer replace)*/
			else if (!strcmp(name, "Replace") || !strcmp(name, "Insert") || !strcmp(name, "Delete") )  {
				if (parser->command_buffer) {
					if (parser->command_buffer->bufferSize) {
						parser->command_buffer->bufferSize--;
					} else {
						SFCommandBuffer *prev = (SFCommandBuffer *) parser->command_buffer->buffer;
						parser->command_buffer->buffer = NULL;
						parser->command_buffer = prev;
					}
					/*stay in command parsing mode (state 3) until we find </buffer>*/
					parser->state = XMT_STATE_COMMANDS;
				}
			}
			/*end of protofield node(s) content*/
			else if (!strcmp(name, "node") || !strcmp(name, "nodes")) {
				top->container_field.far_ptr = NULL;
				top->container_field.name = NULL;
				top->last = NULL;
			}
		}
		/*SF/MFNode proto field, just pop node stack*/
		else if (!top->node && !strcmp(name, "field")) {
			gf_list_rem_last(parser->nodes);
			gf_free(top);
		} else if (top->node && top->node->sgprivate->tag == TAG_ProtoNode) {
			if (!strcmp(name, "node") || !strcmp(name, "nodes")) {
				top->container_field.far_ptr = NULL;
				top->container_field.name = NULL;
				top->last = NULL;
			} else if (!strcmp(name, "ProtoInstance")) {
				gf_list_rem_last(parser->nodes);
				node = top->node;
				gf_free(top);
				goto attach_node;
			}
		}
	} else if (top->node->sgprivate->tag==tag) {
		node = top->node;
		gf_list_rem_last(parser->nodes);
		gf_free(top);

attach_node:
		top = (XMTNodeStack*)gf_list_last(parser->nodes);
		/*add node to command*/
		if (!top || (top->container_field.fieldType==GF_SG_VRML_SFCOMMANDBUFFER)) {
			if (parser->doc_type == 1) {
				GF_CommandField *inf;
				Bool single_node = 0;
				if (!parser->command) {
					gf_assert(0);
					return;
				}
				switch (parser->command->tag) {
				case GF_SG_SCENE_REPLACE:
					if (parser->parsing_proto) {
						gf_sg_proto_add_node_code(parser->parsing_proto, node);
						gf_node_register(node, NULL);
					} else if (!parser->command->node) {
						parser->command->node = node;
						gf_node_register(node, NULL);
					} else if (parser->command->node != node) {
						xmt_report(parser, GF_OK, "Warning: top-node already assigned - discarding node %s", name);
						gf_node_register(node, NULL);
						gf_node_unregister(node, NULL);
					}
					break;
				case GF_SG_GLOBAL_QUANTIZER:
				case GF_SG_NODE_INSERT:
				case GF_SG_INDEXED_INSERT:
				case GF_SG_INDEXED_REPLACE:
					single_node = 1;
				case GF_SG_NODE_REPLACE:
				case GF_SG_FIELD_REPLACE:
				case GF_SG_MULTIPLE_REPLACE:
					inf = (GF_CommandField*)gf_list_last(parser->command->command_fields);
					if (!inf) {
						inf = gf_sg_command_field_new(parser->command);
						inf->fieldType = GF_SG_VRML_SFNODE;
					}
					if ((inf->fieldType==GF_SG_VRML_MFNODE) && !inf->node_list) {
						inf->field_ptr = &inf->node_list;
						if (inf->new_node) {
							gf_node_list_add_child(& inf->node_list, inf->new_node);
							inf->new_node = NULL;
						}
					}

					if (inf->new_node) {
						if (single_node) {
							gf_node_unregister(inf->new_node, NULL);
						} else {
							inf->field_ptr = &inf->node_list;
							gf_node_list_add_child(& inf->node_list, inf->new_node);
							inf->fieldType = GF_SG_VRML_MFNODE;
						}
						inf->new_node = NULL;
					}
					gf_node_register(node, NULL);
					if (inf->node_list) {
						gf_node_list_add_child(& inf->node_list, node);
					} else {
						inf->new_node = node;
						inf->field_ptr = &inf->new_node;
					}
					break;
				case GF_SG_PROTO_INSERT:
					if (parser->parsing_proto) {
						gf_sg_proto_add_node_code(parser->parsing_proto, node);
						gf_node_register(node, NULL);
						break;
					}
				default:
					xmt_report(parser, GF_OK, "Warning: node %s defined outside scene scope - skipping", name);
					gf_node_register(node, NULL);
					gf_node_unregister(node, NULL);
					break;

				}
			}
			/*X3D*/
			else if (parser->doc_type == 2) {
				if (parser->parsing_proto) {
					gf_sg_proto_add_node_code(parser->parsing_proto, node);
					gf_node_register(node, NULL);
				} else {
					M_Group *gr = (M_Group *)gf_sg_get_root_node(parser->load->scene_graph);
					if (!gr) {
						xmt_report(parser, GF_OK, "Warning: node %s defined outside scene scope - skipping", name);
						gf_node_register(node, NULL);
						gf_node_unregister(node, NULL);
					} else {
						//node has already been added to its parent with X3d parsing, because of the default container resolving
//						gf_node_list_add_child(& gr->children, node);
//						gf_node_register(node, NULL);
					}
				}
			}
			/*special case: replace scene has already been applied (progressive loading)*/
			else if ((parser->load->flags & GF_SM_LOAD_FOR_PLAYBACK) && (parser->load->scene_graph->RootNode!=node) ) {
				gf_node_register(node, NULL);
			} else {
				xmt_report(parser, GF_OK, "Warning: node %s defined outside scene scope - skipping", name);
				gf_node_register(node, NULL);
				gf_node_unregister(node, NULL);
			}
		}
		if (parser->load->flags & GF_SM_LOAD_FOR_PLAYBACK) {
			/*load scripts*/
			if (!parser->parsing_proto) {
				if ((tag==TAG_MPEG4_Script)
#ifndef GPAC_DISABLE_X3D
				        || (tag==TAG_X3D_Script)
#endif
				   ) {
					/*it may happen that the script uses itself as a field (not sure this is compliant since this
					implies a cyclic structure, but happens in some X3D conformance seq)*/
					if (!top || (top->node != node)) {
						if (parser->command) {
							if (!parser->command->scripts_to_load) parser->command->scripts_to_load = gf_list_new();
							gf_list_add(parser->command->scripts_to_load, node);
						}
						/*do not load script until all routes are established!!*/
						else if (parser->doc_type==2) {
							gf_list_add(parser->script_to_load, node);
						} else {
							gf_sg_script_load(node);
						}
					}
				}
			}
		}
	} else if (parser->current_node_tag==tag) {
		gf_list_rem_last(parser->nodes);
		gf_free(top);
	} else {
		xmt_report(parser, GF_NON_COMPLIANT_BITSTREAM, "Warning: closing element %s doesn't match created node %s", name, gf_node_get_class_name(top->node) );
	}
}