void mesh_query_done(struct mesh_state* mstate)
{
	struct mesh_reply* r;
	struct mesh_reply* prev = NULL;
	struct sldns_buffer* prev_buffer = NULL;
	struct mesh_cb* c;
	struct reply_info* rep = (mstate->s.return_msg?
		mstate->s.return_msg->rep:NULL);
	struct timeval tv = {0, 0};
	int i = 0;
	/* No need for the serve expired timer anymore; we are going to reply. */
	if(mstate->s.serve_expired_data) {
		comm_timer_delete(mstate->s.serve_expired_data->timer);
		mstate->s.serve_expired_data->timer = NULL;
	}
	if(mstate->s.return_rcode == LDNS_RCODE_SERVFAIL ||
		(rep && FLAGS_GET_RCODE(rep->flags) == LDNS_RCODE_SERVFAIL)) {
		/* we are SERVFAILing; check for expired answer here */
		mesh_serve_expired_callback(mstate);
		if((mstate->reply_list || mstate->cb_list)
		&& mstate->s.env->cfg->log_servfail
		&& !mstate->s.env->cfg->val_log_squelch) {
			char* err = errinf_to_str_servfail(&mstate->s);
			if(err) { log_err("%s", err); }
		}
	}
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

		/* if a response-ip address block has been stored the
		 *  information should be logged for each client. */
		if(mstate->s.respip_action_info &&
			mstate->s.respip_action_info->addrinfo) {
			respip_inform_print(mstate->s.respip_action_info,
				r->qname, mstate->s.qinfo.qtype,
				mstate->s.qinfo.qclass, r->local_alias,
				&r->query_reply.client_addr,
				r->query_reply.client_addrlen);
		}

		/* if this query is determined to be dropped during the
		 * mesh processing, this is the point to take that action. */
		if(mstate->s.is_drop) {
			/* briefly set the reply_list to NULL, so that the
			 * tcp req info cleanup routine that calls the mesh
			 * to deregister the meshstate for it is not done
			 * because the list is NULL and also accounting is not
			 * done there, but instead we do that here. */
			struct mesh_reply* reply_list = mstate->reply_list;
			infra_wait_limit_dec(mstate->s.env->infra_cache,
				&r->query_reply, mstate->s.env->cfg);
			mstate->reply_list = NULL;
			comm_point_drop_reply(&r->query_reply);
			mstate->reply_list = reply_list;
		} else {
			struct sldns_buffer* r_buffer = r->query_reply.c->buffer;
			if(r->query_reply.c->tcp_req_info) {
				r_buffer = r->query_reply.c->tcp_req_info->spool_buffer;
				prev_buffer = NULL;
			}
			mesh_send_reply(mstate, mstate->s.return_rcode, rep,
				r, r_buffer, prev, prev_buffer);
			if(r->query_reply.c->tcp_req_info) {
				tcp_req_info_remove_mesh_state(r->query_reply.c->tcp_req_info, mstate);
				r_buffer = NULL;
			}
			prev = r;
			prev_buffer = r_buffer;
		}
	}
	/* Account for each reply sent. */
	if(i > 0 && mstate->s.respip_action_info &&
		mstate->s.respip_action_info->addrinfo &&
		mstate->s.env->cfg->stat_extended &&
		mstate->s.respip_action_info->rpz_used) {
		if(mstate->s.respip_action_info->rpz_disabled)
			mstate->s.env->mesh->rpz_action[RPZ_DISABLED_ACTION] += i;
		if(mstate->s.respip_action_info->rpz_cname_override)
			mstate->s.env->mesh->rpz_action[RPZ_CNAME_OVERRIDE_ACTION] += i;
		else
			mstate->s.env->mesh->rpz_action[respip_action_to_rpz_action(
				mstate->s.respip_action_info->action)] += i;
	}
	if(!mstate->s.is_drop && i > 0) {
		if(mstate->s.env->cfg->stat_extended
			&& mstate->s.is_cachedb_answer) {
			mstate->s.env->mesh->ans_cachedb += i;
		}
	}

	/* Mesh area accounting */
	if(mstate->reply_list) {
		mstate->reply_list = NULL;
		if(!mstate->reply_list && !mstate->cb_list) {
			/* was a reply state, not anymore */
			log_assert(mstate->s.env->mesh->num_reply_states > 0);
			mstate->s.env->mesh->num_reply_states--;
		}
		if(!mstate->reply_list && !mstate->cb_list &&
			mstate->super_set.count == 0)
			mstate->s.env->mesh->num_detached_states++;
	}
	mstate->replies_sent = 1;

	while((c = mstate->cb_list) != NULL) {
		/* take this cb off the list; so that the list can be
		 * changed, eg. by adds from the callback routine */
		if(!mstate->reply_list && mstate->cb_list && !c->next) {
			/* was a reply state, not anymore */
			log_assert(mstate->s.env->mesh->num_reply_states > 0);
			mstate->s.env->mesh->num_reply_states--;
		}
		mstate->cb_list = c->next;
		if(!mstate->reply_list && !mstate->cb_list &&
			mstate->super_set.count == 0)
			mstate->s.env->mesh->num_detached_states++;
		mesh_do_callback(mstate, mstate->s.return_rcode, rep, c, &tv);
	}
}