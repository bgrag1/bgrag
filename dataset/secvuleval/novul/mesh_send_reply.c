mesh_send_reply(struct mesh_state* m, int rcode, struct reply_info* rep,
	struct mesh_reply* r, struct sldns_buffer* r_buffer,
	struct mesh_reply* prev, struct sldns_buffer* prev_buffer)
{
	struct timeval end_time;
	struct timeval duration;
	int secure;
	/* briefly set the replylist to null in case the
	 * meshsendreply calls tcpreqinfo sendreply that
	 * comm_point_drops because of size, and then the
	 * null stops the mesh state remove and thus
	 * reply_list modification and accounting */
	struct mesh_reply* rlist = m->reply_list;

	/* rpz: apply actions */
	rcode = mesh_is_udp(r) && mesh_is_rpz_respip_tcponly_action(m)
			? (rcode|BIT_TC) : rcode;

	/* examine security status */
	if(m->s.env->need_to_validate && (!(r->qflags&BIT_CD) ||
		m->s.env->cfg->ignore_cd) && rep &&
		(rep->security <= sec_status_bogus ||
		rep->security == sec_status_secure_sentinel_fail)) {
		rcode = LDNS_RCODE_SERVFAIL;
		if(m->s.env->cfg->stat_extended)
			m->s.env->mesh->ans_bogus++;
	}
	if(rep && rep->security == sec_status_secure)
		secure = 1;
	else	secure = 0;
	if(!rep && rcode == LDNS_RCODE_NOERROR)
		rcode = LDNS_RCODE_SERVFAIL;
	if(r->query_reply.c->use_h2) {
		r->query_reply.c->h2_stream = r->h2_stream;
		/* Mesh reply won't exist for long anymore. Make it impossible
		 * for HTTP/2 stream to refer to mesh state, in case
		 * connection gets cleanup before HTTP/2 stream close. */
		r->h2_stream->mesh_state = NULL;
	}
	/* send the reply */
	/* We don't reuse the encoded answer if:
	 * - either the previous or current response has a local alias.  We could
	 *   compare the alias records and still reuse the previous answer if they
	 *   are the same, but that would be complicated and error prone for the
	 *   relatively minor case. So we err on the side of safety.
	 * - there are registered callback functions for the given rcode, as these
	 *   need to be called for each reply. */
	if(((rcode != LDNS_RCODE_SERVFAIL &&
			!m->s.env->inplace_cb_lists[inplace_cb_reply]) ||
		(rcode == LDNS_RCODE_SERVFAIL &&
			!m->s.env->inplace_cb_lists[inplace_cb_reply_servfail])) &&
		prev && prev_buffer && prev->qflags == r->qflags &&
		!prev->local_alias && !r->local_alias &&
		prev->edns.edns_present == r->edns.edns_present &&
		prev->edns.bits == r->edns.bits &&
		prev->edns.udp_size == r->edns.udp_size &&
		edns_opt_list_compare(prev->edns.opt_list_out, r->edns.opt_list_out) == 0 &&
		edns_opt_list_compare(prev->edns.opt_list_inplace_cb_out, r->edns.opt_list_inplace_cb_out) == 0
		) {
		/* if the previous reply is identical to this one, fix ID */
		if(prev_buffer != r_buffer)
			sldns_buffer_copy(r_buffer, prev_buffer);
		sldns_buffer_write_at(r_buffer, 0, &r->qid, sizeof(uint16_t));
		sldns_buffer_write_at(r_buffer, 12, r->qname,
			m->s.qinfo.qname_len);
		m->reply_list = NULL;
		comm_point_send_reply(&r->query_reply);
		m->reply_list = rlist;
	} else if(rcode) {
		m->s.qinfo.qname = r->qname;
		m->s.qinfo.local_alias = r->local_alias;
		if(rcode == LDNS_RCODE_SERVFAIL) {
			if(!inplace_cb_reply_servfail_call(m->s.env, &m->s.qinfo, &m->s,
				rep, rcode, &r->edns, &r->query_reply, m->s.region, &r->start_time))
					r->edns.opt_list_inplace_cb_out = NULL;
		} else {
			if(!inplace_cb_reply_call(m->s.env, &m->s.qinfo, &m->s, rep, rcode,
				&r->edns, &r->query_reply, m->s.region, &r->start_time))
					r->edns.opt_list_inplace_cb_out = NULL;
		}
		/* Send along EDE EDNS0 option when SERVFAILing; usually
		 * DNSSEC validation failures */
		/* Since we are SERVFAILing here, CD bit and rep->security
		 * is already handled. */
		if(m->s.env->cfg->ede && rep) {
			mesh_find_and_attach_ede_and_reason(m, rep, r);
		}
		error_encode(r_buffer, rcode, &m->s.qinfo, r->qid,
			r->qflags, &r->edns);
		m->reply_list = NULL;
		comm_point_send_reply(&r->query_reply);
		m->reply_list = rlist;
	} else {
		size_t udp_size = r->edns.udp_size;
		r->edns.edns_version = EDNS_ADVERTISED_VERSION;
		r->edns.udp_size = EDNS_ADVERTISED_SIZE;
		r->edns.ext_rcode = 0;
		r->edns.bits &= EDNS_DO;
		if(m->s.env->cfg->disable_edns_do && (r->edns.bits&EDNS_DO))
			r->edns.edns_present = 0;
		m->s.qinfo.qname = r->qname;
		m->s.qinfo.local_alias = r->local_alias;

		/* Attach EDE without SERVFAIL if the validation failed.
		 * Need to explicitly check for rep->security otherwise failed
		 * validation paths may attach to a secure answer. */
		if(m->s.env->cfg->ede && rep &&
			(rep->security <= sec_status_bogus ||
			rep->security == sec_status_secure_sentinel_fail)) {
			mesh_find_and_attach_ede_and_reason(m, rep, r);
		}

		if(!inplace_cb_reply_call(m->s.env, &m->s.qinfo, &m->s, rep,
			LDNS_RCODE_NOERROR, &r->edns, &r->query_reply, m->s.region, &r->start_time) ||
			!reply_info_answer_encode(&m->s.qinfo, rep, r->qid,
			r->qflags, r_buffer, 0, 1, m->s.env->scratch,
			udp_size, &r->edns, (int)(r->edns.bits & EDNS_DO),
			secure))
		{
			if(!inplace_cb_reply_servfail_call(m->s.env, &m->s.qinfo, &m->s,
			rep, LDNS_RCODE_SERVFAIL, &r->edns, &r->query_reply, m->s.region, &r->start_time))
				r->edns.opt_list_inplace_cb_out = NULL;
			/* internal server error (probably malloc failure) so no
			 * EDE (RFC8914) needed */
			error_encode(r_buffer, LDNS_RCODE_SERVFAIL,
				&m->s.qinfo, r->qid, r->qflags, &r->edns);
		}
		m->reply_list = NULL;
		comm_point_send_reply(&r->query_reply);
		m->reply_list = rlist;
	}
	infra_wait_limit_dec(m->s.env->infra_cache, &r->query_reply,
		m->s.env->cfg);
	/* account */
	log_assert(m->s.env->mesh->num_reply_addrs > 0);
	m->s.env->mesh->num_reply_addrs--;
	end_time = *m->s.env->now_tv;
	timeval_subtract(&duration, &end_time, &r->start_time);
	verbose(VERB_ALGO, "query took " ARG_LL "d.%6.6d sec",
		(long long)duration.tv_sec, (int)duration.tv_usec);
	m->s.env->mesh->replies_sent++;
	timeval_add(&m->s.env->mesh->replies_sum_wait, &duration);
	timehist_insert(m->s.env->mesh->histogram, &duration);
	if(m->s.env->cfg->stat_extended) {
		uint16_t rc = FLAGS_GET_RCODE(sldns_buffer_read_u16_at(
			r_buffer, 2));
		if(secure) m->s.env->mesh->ans_secure++;
		m->s.env->mesh->ans_rcode[ rc ] ++;
		if(rc == 0 && LDNS_ANCOUNT(sldns_buffer_begin(r_buffer)) == 0)
			m->s.env->mesh->ans_nodata++;
	}
	/* Log reply sent */
	if(m->s.env->cfg->log_replies) {
		log_reply_info(NO_VERBOSE, &m->s.qinfo,
			&r->query_reply.client_addr,
			r->query_reply.client_addrlen, duration, 0, r_buffer,
			(m->s.env->cfg->log_destaddr?(void*)r->query_reply.c->socket->addr:NULL),
			r->query_reply.c->type);
	}
}