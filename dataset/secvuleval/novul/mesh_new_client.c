void mesh_new_client(struct mesh_area* mesh, struct query_info* qinfo,
	struct respip_client_info* cinfo, uint16_t qflags,
	struct edns_data* edns, struct comm_reply* rep, uint16_t qid,
	int rpz_passthru)
{
	struct mesh_state* s = NULL;
	int unique = unique_mesh_state(edns->opt_list_in, mesh->env);
	int was_detached = 0;
	int was_noreply = 0;
	int added = 0;
	int timeout = mesh->env->cfg->serve_expired?
		mesh->env->cfg->serve_expired_client_timeout:0;
	struct sldns_buffer* r_buffer = rep->c->buffer;
	if(rep->c->tcp_req_info) {
		r_buffer = rep->c->tcp_req_info->spool_buffer;
	}
	if(!infra_wait_limit_allowed(mesh->env->infra_cache, rep,
		edns->cookie_valid, mesh->env->cfg)) {
		verbose(VERB_ALGO, "Too many queries waiting from the IP. "
			"dropping incoming query.");
		comm_point_drop_reply(rep);
		mesh->stats_dropped++;
		return;
	}
	if(!unique)
		s = mesh_area_find(mesh, cinfo, qinfo, qflags&(BIT_RD|BIT_CD), 0, 0);
	/* does this create a new reply state? */
	if(!s || s->list_select == mesh_no_list) {
		if(!mesh_make_new_space(mesh, rep->c->buffer)) {
			verbose(VERB_ALGO, "Too many queries. dropping "
				"incoming query.");
			comm_point_drop_reply(rep);
			mesh->stats_dropped++;
			return;
		}
		/* for this new reply state, the reply address is free,
		 * so the limit of reply addresses does not stop reply states*/
	} else {
		/* protect our memory usage from storing reply addresses */
		if(mesh->num_reply_addrs > mesh->max_reply_states*16) {
			verbose(VERB_ALGO, "Too many requests queued. "
				"dropping incoming query.");
			comm_point_drop_reply(rep);
			mesh->stats_dropped++;
			return;
		}
	}
	/* see if it already exists, if not, create one */
	if(!s) {
#ifdef UNBOUND_DEBUG
		struct rbnode_type* n;
#endif
		s = mesh_state_create(mesh->env, qinfo, cinfo,
			qflags&(BIT_RD|BIT_CD), 0, 0);
		if(!s) {
			log_err("mesh_state_create: out of memory; SERVFAIL");
			if(!inplace_cb_reply_servfail_call(mesh->env, qinfo, NULL, NULL,
				LDNS_RCODE_SERVFAIL, edns, rep, mesh->env->scratch, mesh->env->now_tv))
					edns->opt_list_inplace_cb_out = NULL;
			error_encode(r_buffer, LDNS_RCODE_SERVFAIL,
				qinfo, qid, qflags, edns);
			comm_point_send_reply(rep);
			return;
		}
		/* set detached (it is now) */
		mesh->num_detached_states++;
		if(unique)
			mesh_state_make_unique(s);
		s->s.rpz_passthru = rpz_passthru;
		/* copy the edns options we got from the front */
		if(edns->opt_list_in) {
			s->s.edns_opts_front_in = edns_opt_copy_region(edns->opt_list_in,
				s->s.region);
			if(!s->s.edns_opts_front_in) {
				log_err("edns_opt_copy_region: out of memory; SERVFAIL");
				if(!inplace_cb_reply_servfail_call(mesh->env, qinfo, NULL,
					NULL, LDNS_RCODE_SERVFAIL, edns, rep, mesh->env->scratch, mesh->env->now_tv))
						edns->opt_list_inplace_cb_out = NULL;
				error_encode(r_buffer, LDNS_RCODE_SERVFAIL,
					qinfo, qid, qflags, edns);
				comm_point_send_reply(rep);
				mesh_state_delete(&s->s);
				return;
			}
		}

#ifdef UNBOUND_DEBUG
		n =
#else
		(void)
#endif
		rbtree_insert(&mesh->all, &s->node);
		log_assert(n != NULL);
		added = 1;
	}
	if(!s->reply_list && !s->cb_list) {
		was_noreply = 1;
		if(s->super_set.count == 0) {
			was_detached = 1;
		}
	}
	/* add reply to s */
	if(!mesh_state_add_reply(s, edns, rep, qid, qflags, qinfo)) {
		log_err("mesh_new_client: out of memory; SERVFAIL");
		goto servfail_mem;
	}
	if(rep->c->tcp_req_info) {
		if(!tcp_req_info_add_meshstate(rep->c->tcp_req_info, mesh, s)) {
			log_err("mesh_new_client: out of memory add tcpreqinfo");
			goto servfail_mem;
		}
	}
	if(rep->c->use_h2) {
		http2_stream_add_meshstate(rep->c->h2_stream, mesh, s);
	}
	/* add serve expired timer if required and not already there */
	if(timeout && !mesh_serve_expired_init(s, timeout)) {
		log_err("mesh_new_client: out of memory initializing serve expired");
		goto servfail_mem;
	}
#ifdef USE_CACHEDB
	if(!timeout && mesh->env->cfg->serve_expired &&
		!mesh->env->cfg->serve_expired_client_timeout &&
		(mesh->env->cachedb_enabled &&
		 mesh->env->cfg->cachedb_check_when_serve_expired)) {
		if(!mesh_serve_expired_init(s, -1)) {
			log_err("mesh_new_client: out of memory initializing serve expired");
			goto servfail_mem;
		}
	}
#endif
	infra_wait_limit_inc(mesh->env->infra_cache, rep, *mesh->env->now,
		mesh->env->cfg);
	/* update statistics */
	if(was_detached) {
		log_assert(mesh->num_detached_states > 0);
		mesh->num_detached_states--;
	}
	if(was_noreply) {
		mesh->num_reply_states ++;
	}
	mesh->num_reply_addrs++;
	if(s->list_select == mesh_no_list) {
		/* move to either the forever or the jostle_list */
		if(mesh->num_forever_states < mesh->max_forever_states) {
			mesh->num_forever_states ++;
			mesh_list_insert(s, &mesh->forever_first,
				&mesh->forever_last);
			s->list_select = mesh_forever_list;
		} else {
			mesh_list_insert(s, &mesh->jostle_first,
				&mesh->jostle_last);
			s->list_select = mesh_jostle_list;
		}
	}
	if(added)
		mesh_run(mesh, s, module_event_new, NULL);
	return;

servfail_mem:
	if(!inplace_cb_reply_servfail_call(mesh->env, qinfo, &s->s,
		NULL, LDNS_RCODE_SERVFAIL, edns, rep, mesh->env->scratch, mesh->env->now_tv))
			edns->opt_list_inplace_cb_out = NULL;
	error_encode(r_buffer, LDNS_RCODE_SERVFAIL,
		qinfo, qid, qflags, edns);
	comm_point_send_reply(rep);
	if(added)
		mesh_state_delete(&s->s);
	return;
}