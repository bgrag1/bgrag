int z_erofs_parse_cfgs(struct super_block *sb, struct erofs_super_block *dsb)
{
	struct erofs_sb_info *sbi = EROFS_SB(sb);
	struct erofs_buf buf = __EROFS_BUF_INITIALIZER;
	unsigned int algs, alg;
	erofs_off_t offset;
	int size, ret = 0;

	if (!erofs_sb_has_compr_cfgs(sbi)) {
		sbi->available_compr_algs = Z_EROFS_COMPRESSION_LZ4;
		return z_erofs_load_lz4_config(sb, dsb, NULL, 0);
	}

	sbi->available_compr_algs = le16_to_cpu(dsb->u1.available_compr_algs);
	if (sbi->available_compr_algs & ~Z_EROFS_ALL_COMPR_ALGS) {
		erofs_err(sb, "unidentified algorithms %x, please upgrade kernel",
			  sbi->available_compr_algs & ~Z_EROFS_ALL_COMPR_ALGS);
		return -EOPNOTSUPP;
	}

	erofs_init_metabuf(&buf, sb);
	offset = EROFS_SUPER_OFFSET + sbi->sb_size;
	alg = 0;
	for (algs = sbi->available_compr_algs; algs; algs >>= 1, ++alg) {
		void *data;

		if (!(algs & 1))
			continue;

		data = erofs_read_metadata(sb, &buf, &offset, &size);
		if (IS_ERR(data)) {
			ret = PTR_ERR(data);
			break;
		}

		if (alg >= ARRAY_SIZE(erofs_decompressors) ||
		    !erofs_decompressors[alg].config) {
			erofs_err(sb, "algorithm %d isn't enabled on this kernel",
				  alg);
			ret = -EOPNOTSUPP;
		} else {
			ret = erofs_decompressors[alg].config(sb,
					dsb, data, size);
		}

		kfree(data);
		if (ret)
			break;
	}
	erofs_put_metabuf(&buf);
	return ret;
}
