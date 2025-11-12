static int bpf_tracing_prog_attach(struct bpf_prog *prog,
				   int tgt_prog_fd,
				   u32 btf_id,
				   u64 bpf_cookie)
{
	struct bpf_link_primer link_primer;
	struct bpf_prog *tgt_prog = NULL;
	struct bpf_trampoline *tr = NULL;
	struct bpf_tracing_link *link;
	u64 key = 0;
	int err;

	switch (prog->type) {
	case BPF_PROG_TYPE_TRACING:
		if (prog->expected_attach_type != BPF_TRACE_FENTRY &&
		    prog->expected_attach_type != BPF_TRACE_FEXIT &&
		    prog->expected_attach_type != BPF_MODIFY_RETURN) {
			err = -EINVAL;
			goto out_put_prog;
		}
		break;
	case BPF_PROG_TYPE_EXT:
		if (prog->expected_attach_type != 0) {
			err = -EINVAL;
			goto out_put_prog;
		}
		break;
	case BPF_PROG_TYPE_LSM:
		if (prog->expected_attach_type != BPF_LSM_MAC) {
			err = -EINVAL;
			goto out_put_prog;
		}
		break;
	default:
		err = -EINVAL;
		goto out_put_prog;
	}

	if (!!tgt_prog_fd != !!btf_id) {
		err = -EINVAL;
		goto out_put_prog;
	}

	if (tgt_prog_fd) {
		/*
		 * For now we only allow new targets for BPF_PROG_TYPE_EXT. If this
		 * part would be changed to implement the same for
		 * BPF_PROG_TYPE_TRACING, do not forget to update the way how
		 * attach_tracing_prog flag is set.
		 */
		if (prog->type != BPF_PROG_TYPE_EXT) {
			err = -EINVAL;
			goto out_put_prog;
		}

		tgt_prog = bpf_prog_get(tgt_prog_fd);
		if (IS_ERR(tgt_prog)) {
			err = PTR_ERR(tgt_prog);
			tgt_prog = NULL;
			goto out_put_prog;
		}

		key = bpf_trampoline_compute_key(tgt_prog, NULL, btf_id);
	}

	link = kzalloc(sizeof(*link), GFP_USER);
	if (!link) {
		err = -ENOMEM;
		goto out_put_prog;
	}
	bpf_link_init(&link->link.link, BPF_LINK_TYPE_TRACING,
		      &bpf_tracing_link_lops, prog);
	link->attach_type = prog->expected_attach_type;
	link->link.cookie = bpf_cookie;

	mutex_lock(&prog->aux->dst_mutex);

	/* There are a few possible cases here:
	 *
	 * - if prog->aux->dst_trampoline is set, the program was just loaded
	 *   and not yet attached to anything, so we can use the values stored
	 *   in prog->aux
	 *
	 * - if prog->aux->dst_trampoline is NULL, the program has already been
         *   attached to a target and its initial target was cleared (below)
	 *
	 * - if tgt_prog != NULL, the caller specified tgt_prog_fd +
	 *   target_btf_id using the link_create API.
	 *
	 * - if tgt_prog == NULL when this function was called using the old
	 *   raw_tracepoint_open API, and we need a target from prog->aux
	 *
	 * - if prog->aux->dst_trampoline and tgt_prog is NULL, the program
	 *   was detached and is going for re-attachment.
	 *
	 * - if prog->aux->dst_trampoline is NULL and tgt_prog and prog->aux->attach_btf
	 *   are NULL, then program was already attached and user did not provide
	 *   tgt_prog_fd so we have no way to find out or create trampoline
	 */
	if (!prog->aux->dst_trampoline && !tgt_prog) {
		/*
		 * Allow re-attach for TRACING and LSM programs. If it's
		 * currently linked, bpf_trampoline_link_prog will fail.
		 * EXT programs need to specify tgt_prog_fd, so they
		 * re-attach in separate code path.
		 */
		if (prog->type != BPF_PROG_TYPE_TRACING &&
		    prog->type != BPF_PROG_TYPE_LSM) {
			err = -EINVAL;
			goto out_unlock;
		}
		/* We can allow re-attach only if we have valid attach_btf. */
		if (!prog->aux->attach_btf) {
			err = -EINVAL;
			goto out_unlock;
		}
		btf_id = prog->aux->attach_btf_id;
		key = bpf_trampoline_compute_key(NULL, prog->aux->attach_btf, btf_id);
	}

	if (!prog->aux->dst_trampoline ||
	    (key && key != prog->aux->dst_trampoline->key)) {
		/* If there is no saved target, or the specified target is
		 * different from the destination specified at load time, we
		 * need a new trampoline and a check for compatibility
		 */
		struct bpf_attach_target_info tgt_info = {};

		err = bpf_check_attach_target(NULL, prog, tgt_prog, btf_id,
					      &tgt_info);
		if (err)
			goto out_unlock;

		if (tgt_info.tgt_mod) {
			module_put(prog->aux->mod);
			prog->aux->mod = tgt_info.tgt_mod;
		}

		tr = bpf_trampoline_get(key, &tgt_info);
		if (!tr) {
			err = -ENOMEM;
			goto out_unlock;
		}
	} else {
		/* The caller didn't specify a target, or the target was the
		 * same as the destination supplied during program load. This
		 * means we can reuse the trampoline and reference from program
		 * load time, and there is no need to allocate a new one. This
		 * can only happen once for any program, as the saved values in
		 * prog->aux are cleared below.
		 */
		tr = prog->aux->dst_trampoline;
		tgt_prog = prog->aux->dst_prog;
	}

	err = bpf_link_prime(&link->link.link, &link_primer);
	if (err)
		goto out_unlock;

	err = bpf_trampoline_link_prog(&link->link, tr);
	if (err) {
		bpf_link_cleanup(&link_primer);
		link = NULL;
		goto out_unlock;
	}

	link->tgt_prog = tgt_prog;
	link->trampoline = tr;

	/* Always clear the trampoline and target prog from prog->aux to make
	 * sure the original attach destination is not kept alive after a
	 * program is (re-)attached to another target.
	 */
	if (prog->aux->dst_prog &&
	    (tgt_prog_fd || tr != prog->aux->dst_trampoline))
		/* got extra prog ref from syscall, or attaching to different prog */
		bpf_prog_put(prog->aux->dst_prog);
	if (prog->aux->dst_trampoline && tr != prog->aux->dst_trampoline)
		/* we allocated a new trampoline, so free the old one */
		bpf_trampoline_put(prog->aux->dst_trampoline);

	prog->aux->dst_prog = NULL;
	prog->aux->dst_trampoline = NULL;
	mutex_unlock(&prog->aux->dst_mutex);

	return bpf_link_settle(&link_primer);
out_unlock:
	if (tr && tr != prog->aux->dst_trampoline)
		bpf_trampoline_put(tr);
	mutex_unlock(&prog->aux->dst_mutex);
	kfree(link);
out_put_prog:
	if (tgt_prog_fd && tgt_prog)
		bpf_prog_put(tgt_prog);
	return err;
}
