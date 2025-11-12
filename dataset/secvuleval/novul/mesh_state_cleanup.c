mesh_state_cleanup(struct mesh_state* mstate)
{
	struct mesh_area* mesh;
	int i;
	if(!mstate)
		return;
	mesh = mstate->s.env->mesh;
	/* Stop and delete the serve expired timer */
	if(mstate->s.serve_expired_data && mstate->s.serve_expired_data->timer) {
		comm_timer_delete(mstate->s.serve_expired_data->timer);
		mstate->s.serve_expired_data->timer = NULL;
	}
	/* drop unsent replies */
	if(!mstate->replies_sent) {
		struct mesh_reply* rep = mstate->reply_list;
		struct mesh_cb* cb;
		/* in tcp_req_info, the mstates linked are removed, but
		 * the reply_list is now NULL, so the remove-from-empty-list
		 * takes no time and also it does not do the mesh accounting */
		mstate->reply_list = NULL;
		for(; rep; rep=rep->next) {
			infra_wait_limit_dec(mesh->env->infra_cache,
				&rep->query_reply, mesh->env->cfg);
			comm_point_drop_reply(&rep->query_reply);
			log_assert(mesh->num_reply_addrs > 0);
			mesh->num_reply_addrs--;
		}
		while((cb = mstate->cb_list)!=NULL) {
			mstate->cb_list = cb->next;
			fptr_ok(fptr_whitelist_mesh_cb(cb->cb));
			(*cb->cb)(cb->cb_arg, LDNS_RCODE_SERVFAIL, NULL,
				sec_status_unchecked, NULL, 0);
			log_assert(mesh->num_reply_addrs > 0);
			mesh->num_reply_addrs--;
		}
	}

	/* de-init modules */
	for(i=0; i<mesh->mods.num; i++) {
		fptr_ok(fptr_whitelist_mod_clear(mesh->mods.mod[i]->clear));
		(*mesh->mods.mod[i]->clear)(&mstate->s, i);
		mstate->s.minfo[i] = NULL;
		mstate->s.ext_state[i] = module_finished;
	}
	alloc_reg_release(mstate->s.env->alloc, mstate->s.region);
}