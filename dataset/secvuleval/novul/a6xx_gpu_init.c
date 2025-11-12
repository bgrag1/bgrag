struct msm_gpu *a6xx_gpu_init(struct drm_device *dev)
{
	struct msm_drm_private *priv = dev->dev_private;
	struct platform_device *pdev = priv->gpu_pdev;
	struct adreno_platform_config *config = pdev->dev.platform_data;
	struct device_node *node;
	struct a6xx_gpu *a6xx_gpu;
	struct adreno_gpu *adreno_gpu;
	struct msm_gpu *gpu;
	bool is_a7xx;
	int ret;

	a6xx_gpu = kzalloc(sizeof(*a6xx_gpu), GFP_KERNEL);
	if (!a6xx_gpu)
		return ERR_PTR(-ENOMEM);

	adreno_gpu = &a6xx_gpu->base;
	gpu = &adreno_gpu->base;

	mutex_init(&a6xx_gpu->gmu.lock);

	adreno_gpu->registers = NULL;

	/* Check if there is a GMU phandle and set it up */
	node = of_parse_phandle(pdev->dev.of_node, "qcom,gmu", 0);
	/* FIXME: How do we gracefully handle this? */
	BUG_ON(!node);

	adreno_gpu->gmu_is_wrapper = of_device_is_compatible(node, "qcom,adreno-gmu-wrapper");

	adreno_gpu->base.hw_apriv =
		!!(config->info->quirks & ADRENO_QUIRK_HAS_HW_APRIV);

	/* gpu->info only gets assigned in adreno_gpu_init() */
	is_a7xx = config->info->family == ADRENO_7XX_GEN1 ||
		  config->info->family == ADRENO_7XX_GEN2 ||
		  config->info->family == ADRENO_7XX_GEN3;

	a6xx_llc_slices_init(pdev, a6xx_gpu, is_a7xx);

	ret = a6xx_set_supported_hw(&pdev->dev, config->info);
	if (ret) {
		a6xx_llc_slices_destroy(a6xx_gpu);
		kfree(a6xx_gpu);
		return ERR_PTR(ret);
	}

	if (is_a7xx)
		ret = adreno_gpu_init(dev, pdev, adreno_gpu, &funcs_a7xx, 1);
	else if (adreno_has_gmu_wrapper(adreno_gpu))
		ret = adreno_gpu_init(dev, pdev, adreno_gpu, &funcs_gmuwrapper, 1);
	else
		ret = adreno_gpu_init(dev, pdev, adreno_gpu, &funcs, 1);
	if (ret) {
		a6xx_destroy(&(a6xx_gpu->base.base));
		return ERR_PTR(ret);
	}

	/*
	 * For now only clamp to idle freq for devices where this is known not
	 * to cause power supply issues:
	 */
	if (adreno_is_a618(adreno_gpu) || adreno_is_7c3(adreno_gpu))
		priv->gpu_clamp_to_idle = true;

	if (adreno_has_gmu_wrapper(adreno_gpu))
		ret = a6xx_gmu_wrapper_init(a6xx_gpu, node);
	else
		ret = a6xx_gmu_init(a6xx_gpu, node);
	of_node_put(node);
	if (ret) {
		a6xx_destroy(&(a6xx_gpu->base.base));
		return ERR_PTR(ret);
	}

	if (gpu->aspace)
		msm_mmu_set_fault_handler(gpu->aspace->mmu, gpu,
				a6xx_fault_handler);

	a6xx_calc_ubwc_config(adreno_gpu);

	return gpu;
}
