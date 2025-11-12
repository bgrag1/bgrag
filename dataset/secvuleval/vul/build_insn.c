static int build_insn(const struct bpf_insn *insn, struct jit_ctx *ctx, bool extra_pass)
{
	u8 tm = -1;
	u64 func_addr;
	bool func_addr_fixed, sign_extend;
	int i = insn - ctx->prog->insnsi;
	int ret, jmp_offset;
	const u8 code = insn->code;
	const u8 cond = BPF_OP(code);
	const u8 t1 = LOONGARCH_GPR_T1;
	const u8 t2 = LOONGARCH_GPR_T2;
	const u8 src = regmap[insn->src_reg];
	const u8 dst = regmap[insn->dst_reg];
	const s16 off = insn->off;
	const s32 imm = insn->imm;
	const u64 imm64 = (u64)(insn + 1)->imm << 32 | (u32)insn->imm;
	const bool is32 = BPF_CLASS(insn->code) == BPF_ALU || BPF_CLASS(insn->code) == BPF_JMP32;

	switch (code) {
	/* dst = src */
	case BPF_ALU | BPF_MOV | BPF_X:
	case BPF_ALU64 | BPF_MOV | BPF_X:
		switch (off) {
		case 0:
			move_reg(ctx, dst, src);
			emit_zext_32(ctx, dst, is32);
			break;
		case 8:
			move_reg(ctx, t1, src);
			emit_insn(ctx, extwb, dst, t1);
			emit_zext_32(ctx, dst, is32);
			break;
		case 16:
			move_reg(ctx, t1, src);
			emit_insn(ctx, extwh, dst, t1);
			emit_zext_32(ctx, dst, is32);
			break;
		case 32:
			emit_insn(ctx, addw, dst, src, LOONGARCH_GPR_ZERO);
			break;
		}
		break;

	/* dst = imm */
	case BPF_ALU | BPF_MOV | BPF_K:
	case BPF_ALU64 | BPF_MOV | BPF_K:
		move_imm(ctx, dst, imm, is32);
		break;

	/* dst = dst + src */
	case BPF_ALU | BPF_ADD | BPF_X:
	case BPF_ALU64 | BPF_ADD | BPF_X:
		emit_insn(ctx, addd, dst, dst, src);
		emit_zext_32(ctx, dst, is32);
		break;

	/* dst = dst + imm */
	case BPF_ALU | BPF_ADD | BPF_K:
	case BPF_ALU64 | BPF_ADD | BPF_K:
		if (is_signed_imm12(imm)) {
			emit_insn(ctx, addid, dst, dst, imm);
		} else {
			move_imm(ctx, t1, imm, is32);
			emit_insn(ctx, addd, dst, dst, t1);
		}
		emit_zext_32(ctx, dst, is32);
		break;

	/* dst = dst - src */
	case BPF_ALU | BPF_SUB | BPF_X:
	case BPF_ALU64 | BPF_SUB | BPF_X:
		emit_insn(ctx, subd, dst, dst, src);
		emit_zext_32(ctx, dst, is32);
		break;

	/* dst = dst - imm */
	case BPF_ALU | BPF_SUB | BPF_K:
	case BPF_ALU64 | BPF_SUB | BPF_K:
		if (is_signed_imm12(-imm)) {
			emit_insn(ctx, addid, dst, dst, -imm);
		} else {
			move_imm(ctx, t1, imm, is32);
			emit_insn(ctx, subd, dst, dst, t1);
		}
		emit_zext_32(ctx, dst, is32);
		break;

	/* dst = dst * src */
	case BPF_ALU | BPF_MUL | BPF_X:
	case BPF_ALU64 | BPF_MUL | BPF_X:
		emit_insn(ctx, muld, dst, dst, src);
		emit_zext_32(ctx, dst, is32);
		break;

	/* dst = dst * imm */
	case BPF_ALU | BPF_MUL | BPF_K:
	case BPF_ALU64 | BPF_MUL | BPF_K:
		move_imm(ctx, t1, imm, is32);
		emit_insn(ctx, muld, dst, dst, t1);
		emit_zext_32(ctx, dst, is32);
		break;

	/* dst = dst / src */
	case BPF_ALU | BPF_DIV | BPF_X:
	case BPF_ALU64 | BPF_DIV | BPF_X:
		if (!off) {
			emit_zext_32(ctx, dst, is32);
			move_reg(ctx, t1, src);
			emit_zext_32(ctx, t1, is32);
			emit_insn(ctx, divdu, dst, dst, t1);
			emit_zext_32(ctx, dst, is32);
		} else {
			emit_sext_32(ctx, dst, is32);
			move_reg(ctx, t1, src);
			emit_sext_32(ctx, t1, is32);
			emit_insn(ctx, divd, dst, dst, t1);
			emit_sext_32(ctx, dst, is32);
		}
		break;

	/* dst = dst / imm */
	case BPF_ALU | BPF_DIV | BPF_K:
	case BPF_ALU64 | BPF_DIV | BPF_K:
		if (!off) {
			move_imm(ctx, t1, imm, is32);
			emit_zext_32(ctx, dst, is32);
			emit_insn(ctx, divdu, dst, dst, t1);
			emit_zext_32(ctx, dst, is32);
		} else {
			move_imm(ctx, t1, imm, false);
			emit_sext_32(ctx, t1, is32);
			emit_sext_32(ctx, dst, is32);
			emit_insn(ctx, divd, dst, dst, t1);
			emit_sext_32(ctx, dst, is32);
		}
		break;

	/* dst = dst % src */
	case BPF_ALU | BPF_MOD | BPF_X:
	case BPF_ALU64 | BPF_MOD | BPF_X:
		if (!off) {
			emit_zext_32(ctx, dst, is32);
			move_reg(ctx, t1, src);
			emit_zext_32(ctx, t1, is32);
			emit_insn(ctx, moddu, dst, dst, t1);
			emit_zext_32(ctx, dst, is32);
		} else {
			emit_sext_32(ctx, dst, is32);
			move_reg(ctx, t1, src);
			emit_sext_32(ctx, t1, is32);
			emit_insn(ctx, modd, dst, dst, t1);
			emit_sext_32(ctx, dst, is32);
		}
		break;

	/* dst = dst % imm */
	case BPF_ALU | BPF_MOD | BPF_K:
	case BPF_ALU64 | BPF_MOD | BPF_K:
		if (!off) {
			move_imm(ctx, t1, imm, is32);
			emit_zext_32(ctx, dst, is32);
			emit_insn(ctx, moddu, dst, dst, t1);
			emit_zext_32(ctx, dst, is32);
		} else {
			move_imm(ctx, t1, imm, false);
			emit_sext_32(ctx, t1, is32);
			emit_sext_32(ctx, dst, is32);
			emit_insn(ctx, modd, dst, dst, t1);
			emit_sext_32(ctx, dst, is32);
		}
		break;

	/* dst = -dst */
	case BPF_ALU | BPF_NEG:
	case BPF_ALU64 | BPF_NEG:
		move_imm(ctx, t1, imm, is32);
		emit_insn(ctx, subd, dst, LOONGARCH_GPR_ZERO, dst);
		emit_zext_32(ctx, dst, is32);
		break;

	/* dst = dst & src */
	case BPF_ALU | BPF_AND | BPF_X:
	case BPF_ALU64 | BPF_AND | BPF_X:
		emit_insn(ctx, and, dst, dst, src);
		emit_zext_32(ctx, dst, is32);
		break;

	/* dst = dst & imm */
	case BPF_ALU | BPF_AND | BPF_K:
	case BPF_ALU64 | BPF_AND | BPF_K:
		if (is_unsigned_imm12(imm)) {
			emit_insn(ctx, andi, dst, dst, imm);
		} else {
			move_imm(ctx, t1, imm, is32);
			emit_insn(ctx, and, dst, dst, t1);
		}
		emit_zext_32(ctx, dst, is32);
		break;

	/* dst = dst | src */
	case BPF_ALU | BPF_OR | BPF_X:
	case BPF_ALU64 | BPF_OR | BPF_X:
		emit_insn(ctx, or, dst, dst, src);
		emit_zext_32(ctx, dst, is32);
		break;

	/* dst = dst | imm */
	case BPF_ALU | BPF_OR | BPF_K:
	case BPF_ALU64 | BPF_OR | BPF_K:
		if (is_unsigned_imm12(imm)) {
			emit_insn(ctx, ori, dst, dst, imm);
		} else {
			move_imm(ctx, t1, imm, is32);
			emit_insn(ctx, or, dst, dst, t1);
		}
		emit_zext_32(ctx, dst, is32);
		break;

	/* dst = dst ^ src */
	case BPF_ALU | BPF_XOR | BPF_X:
	case BPF_ALU64 | BPF_XOR | BPF_X:
		emit_insn(ctx, xor, dst, dst, src);
		emit_zext_32(ctx, dst, is32);
		break;

	/* dst = dst ^ imm */
	case BPF_ALU | BPF_XOR | BPF_K:
	case BPF_ALU64 | BPF_XOR | BPF_K:
		if (is_unsigned_imm12(imm)) {
			emit_insn(ctx, xori, dst, dst, imm);
		} else {
			move_imm(ctx, t1, imm, is32);
			emit_insn(ctx, xor, dst, dst, t1);
		}
		emit_zext_32(ctx, dst, is32);
		break;

	/* dst = dst << src (logical) */
	case BPF_ALU | BPF_LSH | BPF_X:
		emit_insn(ctx, sllw, dst, dst, src);
		emit_zext_32(ctx, dst, is32);
		break;

	case BPF_ALU64 | BPF_LSH | BPF_X:
		emit_insn(ctx, slld, dst, dst, src);
		break;

	/* dst = dst << imm (logical) */
	case BPF_ALU | BPF_LSH | BPF_K:
		emit_insn(ctx, slliw, dst, dst, imm);
		emit_zext_32(ctx, dst, is32);
		break;

	case BPF_ALU64 | BPF_LSH | BPF_K:
		emit_insn(ctx, sllid, dst, dst, imm);
		break;

	/* dst = dst >> src (logical) */
	case BPF_ALU | BPF_RSH | BPF_X:
		emit_insn(ctx, srlw, dst, dst, src);
		emit_zext_32(ctx, dst, is32);
		break;

	case BPF_ALU64 | BPF_RSH | BPF_X:
		emit_insn(ctx, srld, dst, dst, src);
		break;

	/* dst = dst >> imm (logical) */
	case BPF_ALU | BPF_RSH | BPF_K:
		emit_insn(ctx, srliw, dst, dst, imm);
		emit_zext_32(ctx, dst, is32);
		break;

	case BPF_ALU64 | BPF_RSH | BPF_K:
		emit_insn(ctx, srlid, dst, dst, imm);
		break;

	/* dst = dst >> src (arithmetic) */
	case BPF_ALU | BPF_ARSH | BPF_X:
		emit_insn(ctx, sraw, dst, dst, src);
		emit_zext_32(ctx, dst, is32);
		break;

	case BPF_ALU64 | BPF_ARSH | BPF_X:
		emit_insn(ctx, srad, dst, dst, src);
		break;

	/* dst = dst >> imm (arithmetic) */
	case BPF_ALU | BPF_ARSH | BPF_K:
		emit_insn(ctx, sraiw, dst, dst, imm);
		emit_zext_32(ctx, dst, is32);
		break;

	case BPF_ALU64 | BPF_ARSH | BPF_K:
		emit_insn(ctx, sraid, dst, dst, imm);
		break;

	/* dst = BSWAP##imm(dst) */
	case BPF_ALU | BPF_END | BPF_FROM_LE:
		switch (imm) {
		case 16:
			/* zero-extend 16 bits into 64 bits */
			emit_insn(ctx, bstrpickd, dst, dst, 15, 0);
			break;
		case 32:
			/* zero-extend 32 bits into 64 bits */
			emit_zext_32(ctx, dst, is32);
			break;
		case 64:
			/* do nothing */
			break;
		}
		break;

	case BPF_ALU | BPF_END | BPF_FROM_BE:
	case BPF_ALU64 | BPF_END | BPF_FROM_LE:
		switch (imm) {
		case 16:
			emit_insn(ctx, revb2h, dst, dst);
			/* zero-extend 16 bits into 64 bits */
			emit_insn(ctx, bstrpickd, dst, dst, 15, 0);
			break;
		case 32:
			emit_insn(ctx, revb2w, dst, dst);
			/* clear the upper 32 bits */
			emit_zext_32(ctx, dst, true);
			break;
		case 64:
			emit_insn(ctx, revbd, dst, dst);
			break;
		}
		break;

	/* PC += off if dst cond src */
	case BPF_JMP | BPF_JEQ | BPF_X:
	case BPF_JMP | BPF_JNE | BPF_X:
	case BPF_JMP | BPF_JGT | BPF_X:
	case BPF_JMP | BPF_JGE | BPF_X:
	case BPF_JMP | BPF_JLT | BPF_X:
	case BPF_JMP | BPF_JLE | BPF_X:
	case BPF_JMP | BPF_JSGT | BPF_X:
	case BPF_JMP | BPF_JSGE | BPF_X:
	case BPF_JMP | BPF_JSLT | BPF_X:
	case BPF_JMP | BPF_JSLE | BPF_X:
	case BPF_JMP32 | BPF_JEQ | BPF_X:
	case BPF_JMP32 | BPF_JNE | BPF_X:
	case BPF_JMP32 | BPF_JGT | BPF_X:
	case BPF_JMP32 | BPF_JGE | BPF_X:
	case BPF_JMP32 | BPF_JLT | BPF_X:
	case BPF_JMP32 | BPF_JLE | BPF_X:
	case BPF_JMP32 | BPF_JSGT | BPF_X:
	case BPF_JMP32 | BPF_JSGE | BPF_X:
	case BPF_JMP32 | BPF_JSLT | BPF_X:
	case BPF_JMP32 | BPF_JSLE | BPF_X:
		jmp_offset = bpf2la_offset(i, off, ctx);
		move_reg(ctx, t1, dst);
		move_reg(ctx, t2, src);
		if (is_signed_bpf_cond(BPF_OP(code))) {
			emit_sext_32(ctx, t1, is32);
			emit_sext_32(ctx, t2, is32);
		} else {
			emit_zext_32(ctx, t1, is32);
			emit_zext_32(ctx, t2, is32);
		}
		if (emit_cond_jmp(ctx, cond, t1, t2, jmp_offset) < 0)
			goto toofar;
		break;

	/* PC += off if dst cond imm */
	case BPF_JMP | BPF_JEQ | BPF_K:
	case BPF_JMP | BPF_JNE | BPF_K:
	case BPF_JMP | BPF_JGT | BPF_K:
	case BPF_JMP | BPF_JGE | BPF_K:
	case BPF_JMP | BPF_JLT | BPF_K:
	case BPF_JMP | BPF_JLE | BPF_K:
	case BPF_JMP | BPF_JSGT | BPF_K:
	case BPF_JMP | BPF_JSGE | BPF_K:
	case BPF_JMP | BPF_JSLT | BPF_K:
	case BPF_JMP | BPF_JSLE | BPF_K:
	case BPF_JMP32 | BPF_JEQ | BPF_K:
	case BPF_JMP32 | BPF_JNE | BPF_K:
	case BPF_JMP32 | BPF_JGT | BPF_K:
	case BPF_JMP32 | BPF_JGE | BPF_K:
	case BPF_JMP32 | BPF_JLT | BPF_K:
	case BPF_JMP32 | BPF_JLE | BPF_K:
	case BPF_JMP32 | BPF_JSGT | BPF_K:
	case BPF_JMP32 | BPF_JSGE | BPF_K:
	case BPF_JMP32 | BPF_JSLT | BPF_K:
	case BPF_JMP32 | BPF_JSLE | BPF_K:
		jmp_offset = bpf2la_offset(i, off, ctx);
		if (imm) {
			move_imm(ctx, t1, imm, false);
			tm = t1;
		} else {
			/* If imm is 0, simply use zero register. */
			tm = LOONGARCH_GPR_ZERO;
		}
		move_reg(ctx, t2, dst);
		if (is_signed_bpf_cond(BPF_OP(code))) {
			emit_sext_32(ctx, tm, is32);
			emit_sext_32(ctx, t2, is32);
		} else {
			emit_zext_32(ctx, tm, is32);
			emit_zext_32(ctx, t2, is32);
		}
		if (emit_cond_jmp(ctx, cond, t2, tm, jmp_offset) < 0)
			goto toofar;
		break;

	/* PC += off if dst & src */
	case BPF_JMP | BPF_JSET | BPF_X:
	case BPF_JMP32 | BPF_JSET | BPF_X:
		jmp_offset = bpf2la_offset(i, off, ctx);
		emit_insn(ctx, and, t1, dst, src);
		emit_zext_32(ctx, t1, is32);
		if (emit_cond_jmp(ctx, cond, t1, LOONGARCH_GPR_ZERO, jmp_offset) < 0)
			goto toofar;
		break;

	/* PC += off if dst & imm */
	case BPF_JMP | BPF_JSET | BPF_K:
	case BPF_JMP32 | BPF_JSET | BPF_K:
		jmp_offset = bpf2la_offset(i, off, ctx);
		move_imm(ctx, t1, imm, is32);
		emit_insn(ctx, and, t1, dst, t1);
		emit_zext_32(ctx, t1, is32);
		if (emit_cond_jmp(ctx, cond, t1, LOONGARCH_GPR_ZERO, jmp_offset) < 0)
			goto toofar;
		break;

	/* PC += off */
	case BPF_JMP | BPF_JA:
	case BPF_JMP32 | BPF_JA:
		if (BPF_CLASS(code) == BPF_JMP)
			jmp_offset = bpf2la_offset(i, off, ctx);
		else
			jmp_offset = bpf2la_offset(i, imm, ctx);
		if (emit_uncond_jmp(ctx, jmp_offset) < 0)
			goto toofar;
		break;

	/* function call */
	case BPF_JMP | BPF_CALL:
		mark_call(ctx);
		ret = bpf_jit_get_func_addr(ctx->prog, insn, extra_pass,
					    &func_addr, &func_addr_fixed);
		if (ret < 0)
			return ret;

		move_addr(ctx, t1, func_addr);
		emit_insn(ctx, jirl, t1, LOONGARCH_GPR_RA, 0);
		move_reg(ctx, regmap[BPF_REG_0], LOONGARCH_GPR_A0);
		break;

	/* tail call */
	case BPF_JMP | BPF_TAIL_CALL:
		mark_tail_call(ctx);
		if (emit_bpf_tail_call(ctx) < 0)
			return -EINVAL;
		break;

	/* function return */
	case BPF_JMP | BPF_EXIT:
		if (i == ctx->prog->len - 1)
			break;

		jmp_offset = epilogue_offset(ctx);
		if (emit_uncond_jmp(ctx, jmp_offset) < 0)
			goto toofar;
		break;

	/* dst = imm64 */
	case BPF_LD | BPF_IMM | BPF_DW:
		move_imm(ctx, dst, imm64, is32);
		return 1;

	/* dst = *(size *)(src + off) */
	case BPF_LDX | BPF_MEM | BPF_B:
	case BPF_LDX | BPF_MEM | BPF_H:
	case BPF_LDX | BPF_MEM | BPF_W:
	case BPF_LDX | BPF_MEM | BPF_DW:
	case BPF_LDX | BPF_PROBE_MEM | BPF_DW:
	case BPF_LDX | BPF_PROBE_MEM | BPF_W:
	case BPF_LDX | BPF_PROBE_MEM | BPF_H:
	case BPF_LDX | BPF_PROBE_MEM | BPF_B:
	/* dst_reg = (s64)*(signed size *)(src_reg + off) */
	case BPF_LDX | BPF_MEMSX | BPF_B:
	case BPF_LDX | BPF_MEMSX | BPF_H:
	case BPF_LDX | BPF_MEMSX | BPF_W:
	case BPF_LDX | BPF_PROBE_MEMSX | BPF_B:
	case BPF_LDX | BPF_PROBE_MEMSX | BPF_H:
	case BPF_LDX | BPF_PROBE_MEMSX | BPF_W:
		sign_extend = BPF_MODE(insn->code) == BPF_MEMSX ||
			      BPF_MODE(insn->code) == BPF_PROBE_MEMSX;
		switch (BPF_SIZE(code)) {
		case BPF_B:
			if (is_signed_imm12(off)) {
				if (sign_extend)
					emit_insn(ctx, ldb, dst, src, off);
				else
					emit_insn(ctx, ldbu, dst, src, off);
			} else {
				move_imm(ctx, t1, off, is32);
				if (sign_extend)
					emit_insn(ctx, ldxb, dst, src, t1);
				else
					emit_insn(ctx, ldxbu, dst, src, t1);
			}
			break;
		case BPF_H:
			if (is_signed_imm12(off)) {
				if (sign_extend)
					emit_insn(ctx, ldh, dst, src, off);
				else
					emit_insn(ctx, ldhu, dst, src, off);
			} else {
				move_imm(ctx, t1, off, is32);
				if (sign_extend)
					emit_insn(ctx, ldxh, dst, src, t1);
				else
					emit_insn(ctx, ldxhu, dst, src, t1);
			}
			break;
		case BPF_W:
			if (is_signed_imm12(off)) {
				if (sign_extend)
					emit_insn(ctx, ldw, dst, src, off);
				else
					emit_insn(ctx, ldwu, dst, src, off);
			} else {
				move_imm(ctx, t1, off, is32);
				if (sign_extend)
					emit_insn(ctx, ldxw, dst, src, t1);
				else
					emit_insn(ctx, ldxwu, dst, src, t1);
			}
			break;
		case BPF_DW:
			move_imm(ctx, t1, off, is32);
			emit_insn(ctx, ldxd, dst, src, t1);
			break;
		}

		ret = add_exception_handler(insn, ctx, dst);
		if (ret)
			return ret;
		break;

	/* *(size *)(dst + off) = imm */
	case BPF_ST | BPF_MEM | BPF_B:
	case BPF_ST | BPF_MEM | BPF_H:
	case BPF_ST | BPF_MEM | BPF_W:
	case BPF_ST | BPF_MEM | BPF_DW:
		switch (BPF_SIZE(code)) {
		case BPF_B:
			move_imm(ctx, t1, imm, is32);
			if (is_signed_imm12(off)) {
				emit_insn(ctx, stb, t1, dst, off);
			} else {
				move_imm(ctx, t2, off, is32);
				emit_insn(ctx, stxb, t1, dst, t2);
			}
			break;
		case BPF_H:
			move_imm(ctx, t1, imm, is32);
			if (is_signed_imm12(off)) {
				emit_insn(ctx, sth, t1, dst, off);
			} else {
				move_imm(ctx, t2, off, is32);
				emit_insn(ctx, stxh, t1, dst, t2);
			}
			break;
		case BPF_W:
			move_imm(ctx, t1, imm, is32);
			if (is_signed_imm12(off)) {
				emit_insn(ctx, stw, t1, dst, off);
			} else if (is_signed_imm14(off)) {
				emit_insn(ctx, stptrw, t1, dst, off);
			} else {
				move_imm(ctx, t2, off, is32);
				emit_insn(ctx, stxw, t1, dst, t2);
			}
			break;
		case BPF_DW:
			move_imm(ctx, t1, imm, is32);
			if (is_signed_imm12(off)) {
				emit_insn(ctx, std, t1, dst, off);
			} else if (is_signed_imm14(off)) {
				emit_insn(ctx, stptrd, t1, dst, off);
			} else {
				move_imm(ctx, t2, off, is32);
				emit_insn(ctx, stxd, t1, dst, t2);
			}
			break;
		}
		break;

	/* *(size *)(dst + off) = src */
	case BPF_STX | BPF_MEM | BPF_B:
	case BPF_STX | BPF_MEM | BPF_H:
	case BPF_STX | BPF_MEM | BPF_W:
	case BPF_STX | BPF_MEM | BPF_DW:
		switch (BPF_SIZE(code)) {
		case BPF_B:
			if (is_signed_imm12(off)) {
				emit_insn(ctx, stb, src, dst, off);
			} else {
				move_imm(ctx, t1, off, is32);
				emit_insn(ctx, stxb, src, dst, t1);
			}
			break;
		case BPF_H:
			if (is_signed_imm12(off)) {
				emit_insn(ctx, sth, src, dst, off);
			} else {
				move_imm(ctx, t1, off, is32);
				emit_insn(ctx, stxh, src, dst, t1);
			}
			break;
		case BPF_W:
			if (is_signed_imm12(off)) {
				emit_insn(ctx, stw, src, dst, off);
			} else if (is_signed_imm14(off)) {
				emit_insn(ctx, stptrw, src, dst, off);
			} else {
				move_imm(ctx, t1, off, is32);
				emit_insn(ctx, stxw, src, dst, t1);
			}
			break;
		case BPF_DW:
			if (is_signed_imm12(off)) {
				emit_insn(ctx, std, src, dst, off);
			} else if (is_signed_imm14(off)) {
				emit_insn(ctx, stptrd, src, dst, off);
			} else {
				move_imm(ctx, t1, off, is32);
				emit_insn(ctx, stxd, src, dst, t1);
			}
			break;
		}
		break;

	case BPF_STX | BPF_ATOMIC | BPF_W:
	case BPF_STX | BPF_ATOMIC | BPF_DW:
		emit_atomic(insn, ctx);
		break;

	/* Speculation barrier */
	case BPF_ST | BPF_NOSPEC:
		break;

	default:
		pr_err("bpf_jit: unknown opcode %02x\n", code);
		return -EINVAL;
	}

	return 0;

toofar:
	pr_info_once("bpf_jit: opcode %02x, jump too far\n", code);
	return -E2BIG;
}
