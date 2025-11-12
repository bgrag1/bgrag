static inline void nft_reg_store64(u64 *dreg, u64 val)
{
	put_unaligned(val, dreg);
}