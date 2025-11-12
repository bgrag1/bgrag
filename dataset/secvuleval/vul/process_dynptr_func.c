static int process_dynptr_func(struct bpf_verifier_env *env, int regno, int insn_idx,
			       enum bpf_arg_type arg_type, int clone_ref_obj_id)
{
	struct bpf_reg_state *regs = cur_regs(env), *reg = &regs[regno];
	int err;

	/* MEM_UNINIT and MEM_RDONLY are exclusive, when applied to an
	 * ARG_PTR_TO_DYNPTR (or ARG_PTR_TO_DYNPTR | DYNPTR_TYPE_*):
	 */
	if ((arg_type & (MEM_UNINIT | MEM_RDONLY)) == (MEM_UNINIT | MEM_RDONLY)) {
		verbose(env, "verifier internal error: misconfigured dynptr helper type flags\n");
		return -EFAULT;
	}

	/*  MEM_UNINIT - Points to memory that is an appropriate candidate for
	 *		 constructing a mutable bpf_dynptr object.
	 *
	 *		 Currently, this is only possible with PTR_TO_STACK
	 *		 pointing to a region of at least 16 bytes which doesn't
	 *		 contain an existing bpf_dynptr.
	 *
	 *  MEM_RDONLY - Points to a initialized bpf_dynptr that will not be
	 *		 mutated or destroyed. However, the memory it points to
	 *		 may be mutated.
	 *
	 *  None       - Points to a initialized dynptr that can be mutated and
	 *		 destroyed, including mutation of the memory it points
	 *		 to.
	 */
	if (arg_type & MEM_UNINIT) {
		int i;

		if (!is_dynptr_reg_valid_uninit(env, reg)) {
			verbose(env, "Dynptr has to be an uninitialized dynptr\n");
			return -EINVAL;
		}

		/* we write BPF_DW bits (8 bytes) at a time */
		for (i = 0; i < BPF_DYNPTR_SIZE; i += 8) {
			err = check_mem_access(env, insn_idx, regno,
					       i, BPF_DW, BPF_WRITE, -1, false, false);
			if (err)
				return err;
		}

		err = mark_stack_slots_dynptr(env, reg, arg_type, insn_idx, clone_ref_obj_id);
	} else /* MEM_RDONLY and None case from above */ {
		/* For the reg->type == PTR_TO_STACK case, bpf_dynptr is never const */
		if (reg->type == CONST_PTR_TO_DYNPTR && !(arg_type & MEM_RDONLY)) {
			verbose(env, "cannot pass pointer to const bpf_dynptr, the helper mutates it\n");
			return -EINVAL;
		}

		if (!is_dynptr_reg_valid_init(env, reg)) {
			verbose(env,
				"Expected an initialized dynptr as arg #%d\n",
				regno);
			return -EINVAL;
		}

		/* Fold modifiers (in this case, MEM_RDONLY) when checking expected type */
		if (!is_dynptr_type_expected(env, reg, arg_type & ~MEM_RDONLY)) {
			verbose(env,
				"Expected a dynptr of type %s as arg #%d\n",
				dynptr_type_str(arg_to_dynptr_type(arg_type)), regno);
			return -EINVAL;
		}

		err = mark_dynptr_read(env, reg);
	}
	return err;
}
