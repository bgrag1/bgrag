nft_meta_get_eval_time(enum nft_meta_keys key,
		       u32 *dest)
{
	switch (key) {
	case NFT_META_TIME_NS:
		nft_reg_store64((u64 *)dest, ktime_get_real_ns());
		break;
	case NFT_META_TIME_DAY:
		nft_reg_store8(dest, nft_meta_weekday());
		break;
	case NFT_META_TIME_HOUR:
		*dest = nft_meta_hour(ktime_get_real_seconds());
		break;
	default:
		break;
	}
}