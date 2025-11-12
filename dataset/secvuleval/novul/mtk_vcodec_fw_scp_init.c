struct mtk_vcodec_fw *mtk_vcodec_fw_scp_init(void *priv, enum mtk_vcodec_fw_use fw_use)
{
	struct mtk_vcodec_fw *fw;
	struct platform_device *plat_dev;
	struct mtk_scp *scp;

	if (fw_use == ENCODER) {
		struct mtk_vcodec_enc_dev *enc_dev = priv;

		plat_dev = enc_dev->plat_dev;
	} else if (fw_use == DECODER) {
		struct mtk_vcodec_dec_dev *dec_dev = priv;

		plat_dev = dec_dev->plat_dev;
	} else {
		pr_err("Invalid fw_use %d (use a reasonable fw id here)\n", fw_use);
		return ERR_PTR(-EINVAL);
	}

	scp = scp_get(plat_dev);
	if (!scp) {
		dev_err(&plat_dev->dev, "could not get vdec scp handle");
		return ERR_PTR(-EPROBE_DEFER);
	}

	fw = devm_kzalloc(&plat_dev->dev, sizeof(*fw), GFP_KERNEL);
	if (!fw)
		return ERR_PTR(-ENOMEM);
	fw->type = SCP;
	fw->ops = &mtk_vcodec_rproc_msg;
	fw->scp = scp;

	return fw;
}
