static int nft_meta_inner_init(const struct nft_ctx *ctx,
			       const struct nft_expr *expr,
			       const struct nlattr * const tb[])
{
	struct nft_meta *priv = nft_expr_priv(expr);
	unsigned int len;

	priv->key = ntohl(nla_get_be32(tb[NFTA_META_KEY]));
	switch (priv->key) {
	case NFT_META_PROTOCOL:
		len = sizeof(u16);
		break;
	case NFT_META_L4PROTO:
		len = sizeof(u32);
		break;
	default:
		return -EOPNOTSUPP;
	}
	priv->len = len;

	return nft_parse_register_store(ctx, tb[NFTA_META_DREG], &priv->dreg,
					NULL, NFT_DATA_VALUE, len);
}
