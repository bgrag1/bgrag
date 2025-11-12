static int nft_payload_inner_init(const struct nft_ctx *ctx,
				  const struct nft_expr *expr,
				  const struct nlattr * const tb[])
{
	struct nft_payload *priv = nft_expr_priv(expr);
	u32 base;

	base   = ntohl(nla_get_be32(tb[NFTA_PAYLOAD_BASE]));
	switch (base) {
	case NFT_PAYLOAD_TUN_HEADER:
	case NFT_PAYLOAD_LL_HEADER:
	case NFT_PAYLOAD_NETWORK_HEADER:
	case NFT_PAYLOAD_TRANSPORT_HEADER:
		break;
	default:
		return -EOPNOTSUPP;
	}

	priv->base   = base;
	priv->offset = ntohl(nla_get_be32(tb[NFTA_PAYLOAD_OFFSET]));
	priv->len    = ntohl(nla_get_be32(tb[NFTA_PAYLOAD_LEN]));

	return nft_parse_register_store(ctx, tb[NFTA_PAYLOAD_DREG],
					&priv->dreg, NULL, NFT_DATA_VALUE,
					priv->len);
}
