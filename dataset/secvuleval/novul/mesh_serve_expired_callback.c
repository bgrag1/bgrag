mesh_serve_expired_callback(void* arg)
{
	struct mesh_state* mstate = (struct mesh_state*) arg;
	struct module_qstate* qstate = &mstate->s;
	struct mesh_reply* r;
	struct mesh_area* mesh = qstate->env->mesh;
	struct dns_msg* msg;
	struct mesh_cb* c;
	struct mesh_reply* prev = NULL;
	struct sldns_buffer* prev_buffer = NULL;
	struct sldns_buffer* r_buffer = NULL;
	struct reply_info* partial_rep = NULL;
	struct ub_packed_rrset_key* alias_rrset = NULL;
	struct reply_info* encode_rep = NULL;
	struct respip_action_info actinfo;
	struct query_info* lookup_qinfo = &qstate->qinfo;
	struct query_info qinfo_tmp;
	struct timeval tv = {0, 0};
	int must_validate = (!(qstate->query_flags&BIT_CD)
		|| qstate->env->cfg->ignore_cd) && qstate->env->need_to_validate;
	int i = 0;
	if(!qstate->serve_expired_data) return;
	verbose(VERB_ALGO, "Serve expired: Trying to reply with expired data");
	comm_timer_delete(qstate->serve_expired_data->timer);
	qstate->serve_expired_data->timer = NULL;
	/* If is_drop or no_cache_lookup (modules that handle their own cache e.g.,
	 * subnetmod) ignore stale data from the main cache. */
	if(qstate->no_cache_lookup || qstate->is_drop) {
		verbose(VERB_ALGO,
			"Serve expired: Not allowed to look into cache for stale");
		return;
	}
	/* The following while is used instead of the `goto lookup_cache`
	 * like in the worker. */
	while(1) {
		fptr_ok(fptr_whitelist_serve_expired_lookup(
			qstate->serve_expired_data->get_cached_answer));
		msg = (*qstate->serve_expired_data->get_cached_answer)(qstate,
			lookup_qinfo);
		if(!msg)
			return;
		/* Reset these in case we pass a second time from here. */
		encode_rep = msg->rep;
		memset(&actinfo, 0, sizeof(actinfo));
		actinfo.action = respip_none;
		alias_rrset = NULL;
		if((mesh->use_response_ip || mesh->use_rpz) &&
			!partial_rep && !apply_respip_action(qstate, &qstate->qinfo,
			qstate->client_info, &actinfo, msg->rep, &alias_rrset, &encode_rep,
			qstate->env->auth_zones)) {
			return;
		} else if(partial_rep &&
			!respip_merge_cname(partial_rep, &qstate->qinfo, msg->rep,
			qstate->client_info, must_validate, &encode_rep, qstate->region,
			qstate->env->auth_zones)) {
			return;
		}
		if(!encode_rep || alias_rrset) {
			if(!encode_rep) {
				/* Needs drop */
				return;
			} else {
				/* A partial CNAME chain is found. */
				partial_rep = encode_rep;
			}
		}
		/* We've found a partial reply ending with an
		* alias.  Replace the lookup qinfo for the
		* alias target and lookup the cache again to
		* (possibly) complete the reply.  As we're
		* passing the "base" reply, there will be no
		* more alias chasing. */
		if(partial_rep) {
			memset(&qinfo_tmp, 0, sizeof(qinfo_tmp));
			get_cname_target(alias_rrset, &qinfo_tmp.qname,
				&qinfo_tmp.qname_len);
			if(!qinfo_tmp.qname) {
				log_err("Serve expired: unexpected: invalid answer alias");
				return;
			}
			qinfo_tmp.qtype = qstate->qinfo.qtype;
			qinfo_tmp.qclass = qstate->qinfo.qclass;
			lookup_qinfo = &qinfo_tmp;
			continue;
		}
		break;
	}

	if(verbosity >= VERB_ALGO)
		log_dns_msg("Serve expired lookup", &qstate->qinfo, msg->rep);

	for(r = mstate->reply_list; r; r = r->next) {
		struct timeval old;
		timeval_subtract(&old, mstate->s.env->now_tv, &r->start_time);
		if(mstate->s.env->cfg->discard_timeout != 0 &&
			((int)old.tv_sec)*1000+((int)old.tv_usec)/1000 >
			mstate->s.env->cfg->discard_timeout) {
			/* Drop the reply, it is too old */
			/* briefly set the reply_list to NULL, so that the
			 * tcp req info cleanup routine that calls the mesh
			 * to deregister the meshstate for it is not done
			 * because the list is NULL and also accounting is not
			 * done there, but instead we do that here. */
			struct mesh_reply* reply_list = mstate->reply_list;
			verbose(VERB_ALGO, "drop reply, it is older than discard-timeout");
			infra_wait_limit_dec(mstate->s.env->infra_cache,
				&r->query_reply, mstate->s.env->cfg);
			mstate->reply_list = NULL;
			comm_point_drop_reply(&r->query_reply);
			mstate->reply_list = reply_list;
			mstate->s.env->mesh->stats_dropped++;
			continue;
		}

		i++;
		tv = r->start_time;

		/* If address info is returned, it means the action should be an
		* 'inform' variant and the information should be logged. */
		if(actinfo.addrinfo) {
			respip_inform_print(&actinfo, r->qname,
				qstate->qinfo.qtype, qstate->qinfo.qclass,
				r->local_alias, &r->query_reply.client_addr,
				r->query_reply.client_addrlen);
		}

		/* Add EDE Stale Answer (RCF8914). Ignore global ede as this is
		 * warning instead of an error */
		if (r->edns.edns_present && qstate->env->cfg->ede_serve_expired &&
			qstate->env->cfg->ede) {
			edns_opt_list_append_ede(&r->edns.opt_list_out,
				mstate->s.region, LDNS_EDE_STALE_ANSWER, NULL);
		}

		r_buffer = r->query_reply.c->buffer;
		if(r->query_reply.c->tcp_req_info)
			r_buffer = r->query_reply.c->tcp_req_info->spool_buffer;
		mesh_send_reply(mstate, LDNS_RCODE_NOERROR, msg->rep,
			r, r_buffer, prev, prev_buffer);
		if(r->query_reply.c->tcp_req_info)
			tcp_req_info_remove_mesh_state(r->query_reply.c->tcp_req_info, mstate);
		infra_wait_limit_dec(mstate->s.env->infra_cache,
			&r->query_reply, mstate->s.env->cfg);
		prev = r;
		prev_buffer = r_buffer;
	}
	/* Account for each reply sent. */
	if(i > 0) {
		mesh->ans_expired += i;
		if(actinfo.addrinfo && qstate->env->cfg->stat_extended &&
			actinfo.rpz_used) {
			if(actinfo.rpz_disabled)
				qstate->env->mesh->rpz_action[RPZ_DISABLED_ACTION] += i;
			if(actinfo.rpz_cname_override)
				qstate->env->mesh->rpz_action[RPZ_CNAME_OVERRIDE_ACTION] += i;
			else
				qstate->env->mesh->rpz_action[
					respip_action_to_rpz_action(actinfo.action)] += i;
		}
	}

	/* Mesh area accounting */
	if(mstate->reply_list) {
		mstate->reply_list = NULL;
		if(!mstate->reply_list && !mstate->cb_list) {
			log_assert(mesh->num_reply_states > 0);
			mesh->num_reply_states--;
			if(mstate->super_set.count == 0) {
				mesh->num_detached_states++;
			}
		}
	}

	while((c = mstate->cb_list) != NULL) {
		/* take this cb off the list; so that the list can be
		 * changed, eg. by adds from the callback routine */
		if(!mstate->reply_list && mstate->cb_list && !c->next) {
			/* was a reply state, not anymore */
			log_assert(qstate->env->mesh->num_reply_states > 0);
			qstate->env->mesh->num_reply_states--;
		}
		mstate->cb_list = c->next;
		if(!mstate->reply_list && !mstate->cb_list &&
			mstate->super_set.count == 0)
			qstate->env->mesh->num_detached_states++;
		mesh_do_callback(mstate, LDNS_RCODE_NOERROR, msg->rep, c, &tv);
	}
}