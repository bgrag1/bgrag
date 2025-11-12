void mesh_state_remove_reply(struct mesh_area* mesh, struct mesh_state* m,
	struct comm_point* cp)
{
	struct mesh_reply* n, *prev = NULL;
	n = m->reply_list;
	/* when in mesh_cleanup, it sets the reply_list to NULL, so that
	 * there is no accounting twice */
	if(!n) return; /* nothing to remove, also no accounting needed */
	while(n) {
		if(n->query_reply.c == cp) {
			/* unlink it */
			if(prev) prev->next = n->next;
			else m->reply_list = n->next;
			/* delete it, but allocated in m region */
			log_assert(mesh->num_reply_addrs > 0);
			mesh->num_reply_addrs--;
			infra_wait_limit_dec(mesh->env->infra_cache,
				&n->query_reply, mesh->env->cfg);

			/* prev = prev; */
			n = n->next;
			continue;
		}
		prev = n;
		n = n->next;
	}
	/* it was not detached (because it had a reply list), could be now */
	if(!m->reply_list && !m->cb_list
		&& m->super_set.count == 0) {
		mesh->num_detached_states++;
	}
	/* if not replies any more in mstate, it is no longer a reply_state */
	if(!m->reply_list && !m->cb_list) {
		log_assert(mesh->num_reply_states > 0);
		mesh->num_reply_states--;
	}
}