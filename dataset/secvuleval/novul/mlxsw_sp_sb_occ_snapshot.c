int mlxsw_sp_sb_occ_snapshot(struct mlxsw_core *mlxsw_core,
			     unsigned int sb_index)
{
	u16 local_port, local_port_1, first_local_port, last_local_port;
	struct mlxsw_sp *mlxsw_sp = mlxsw_core_driver_priv(mlxsw_core);
	struct mlxsw_sp_sb_sr_occ_query_cb_ctx cb_ctx;
	u8 masked_count, current_page = 0;
	unsigned long cb_priv = 0;
	LIST_HEAD(bulk_list);
	char *sbsr_pl;
	int i;
	int err;
	int err2;

	sbsr_pl = kmalloc(MLXSW_REG_SBSR_LEN, GFP_KERNEL);
	if (!sbsr_pl)
		return -ENOMEM;

	local_port = MLXSW_PORT_CPU_PORT;
next_batch:
	local_port_1 = local_port;
	masked_count = 0;
	mlxsw_reg_sbsr_pack(sbsr_pl, false);
	mlxsw_reg_sbsr_port_page_set(sbsr_pl, current_page);
	first_local_port = current_page * MLXSW_REG_SBSR_NUM_PORTS_IN_PAGE;
	last_local_port = current_page * MLXSW_REG_SBSR_NUM_PORTS_IN_PAGE +
			  MLXSW_REG_SBSR_NUM_PORTS_IN_PAGE - 1;

	for (i = 0; i < MLXSW_SP_SB_ING_TC_COUNT; i++)
		mlxsw_reg_sbsr_pg_buff_mask_set(sbsr_pl, i, 1);
	for (i = 0; i < MLXSW_SP_SB_EG_TC_COUNT; i++)
		mlxsw_reg_sbsr_tclass_mask_set(sbsr_pl, i, 1);
	for (; local_port < mlxsw_core_max_ports(mlxsw_core); local_port++) {
		if (!mlxsw_sp->ports[local_port])
			continue;
		if (local_port > last_local_port) {
			current_page++;
			goto do_query;
		}
		if (local_port != MLXSW_PORT_CPU_PORT) {
			/* Ingress quotas are not supported for the CPU port */
			mlxsw_reg_sbsr_ingress_port_mask_set(sbsr_pl,
							     local_port - first_local_port,
							     1);
		}
		mlxsw_reg_sbsr_egress_port_mask_set(sbsr_pl,
						    local_port - first_local_port,
						    1);
		for (i = 0; i < mlxsw_sp->sb_vals->pool_count; i++) {
			err = mlxsw_sp_sb_pm_occ_query(mlxsw_sp, local_port, i,
						       &bulk_list);
			if (err)
				goto out;
		}
		if (++masked_count == MASKED_COUNT_MAX)
			goto do_query;
	}

do_query:
	cb_ctx.masked_count = masked_count;
	cb_ctx.local_port_1 = local_port_1;
	memcpy(&cb_priv, &cb_ctx, sizeof(cb_ctx));
	err = mlxsw_reg_trans_query(mlxsw_core, MLXSW_REG(sbsr), sbsr_pl,
				    &bulk_list, mlxsw_sp_sb_sr_occ_query_cb,
				    cb_priv);
	if (err)
		goto out;
	if (local_port < mlxsw_core_max_ports(mlxsw_core)) {
		local_port++;
		goto next_batch;
	}

out:
	err2 = mlxsw_reg_trans_bulk_wait(&bulk_list);
	if (!err)
		err = err2;
	kfree(sbsr_pl);
	return err;
}
