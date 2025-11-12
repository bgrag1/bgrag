int mlxsw_sp_acl_tcam_init(struct mlxsw_sp *mlxsw_sp,
			   struct mlxsw_sp_acl_tcam *tcam)
{
	const struct mlxsw_sp_acl_tcam_ops *ops = mlxsw_sp->acl_tcam_ops;
	u64 max_tcam_regions;
	u64 max_regions;
	u64 max_groups;
	int err;

	mutex_init(&tcam->lock);
	tcam->vregion_rehash_intrvl =
			MLXSW_SP_ACL_TCAM_VREGION_REHASH_INTRVL_DFLT;
	INIT_LIST_HEAD(&tcam->vregion_list);

	err = mlxsw_sp_acl_tcam_rehash_params_register(mlxsw_sp);
	if (err)
		goto err_rehash_params_register;

	max_tcam_regions = MLXSW_CORE_RES_GET(mlxsw_sp->core,
					      ACL_MAX_TCAM_REGIONS);
	max_regions = MLXSW_CORE_RES_GET(mlxsw_sp->core, ACL_MAX_REGIONS);

	/* Use 1:1 mapping between ACL region and TCAM region */
	if (max_tcam_regions < max_regions)
		max_regions = max_tcam_regions;

	tcam->used_regions = bitmap_zalloc(max_regions, GFP_KERNEL);
	if (!tcam->used_regions) {
		err = -ENOMEM;
		goto err_alloc_used_regions;
	}
	tcam->max_regions = max_regions;

	max_groups = MLXSW_CORE_RES_GET(mlxsw_sp->core, ACL_MAX_GROUPS);
	tcam->used_groups = bitmap_zalloc(max_groups, GFP_KERNEL);
	if (!tcam->used_groups) {
		err = -ENOMEM;
		goto err_alloc_used_groups;
	}
	tcam->max_groups = max_groups;
	tcam->max_group_size = MLXSW_CORE_RES_GET(mlxsw_sp->core,
						  ACL_MAX_GROUP_SIZE);
	tcam->max_group_size = min_t(unsigned int, tcam->max_group_size,
				     MLXSW_REG_PAGT_ACL_MAX_NUM);

	err = ops->init(mlxsw_sp, tcam->priv, tcam);
	if (err)
		goto err_tcam_init;

	return 0;

err_tcam_init:
	bitmap_free(tcam->used_groups);
err_alloc_used_groups:
	bitmap_free(tcam->used_regions);
err_alloc_used_regions:
	mlxsw_sp_acl_tcam_rehash_params_unregister(mlxsw_sp);
err_rehash_params_register:
	mutex_destroy(&tcam->lock);
	return err;
}
