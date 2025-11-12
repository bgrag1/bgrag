static int rx_create(struct mlx5_core_dev *mdev, struct mlx5e_ipsec *ipsec,
		     struct mlx5e_ipsec_rx *rx, u32 family)
{
	struct mlx5e_ipsec_rx_create_attr attr;
	struct mlx5_flow_destination dest[2];
	struct mlx5_flow_table *ft;
	u32 flags = 0;
	int err;

	ipsec_rx_create_attr_set(ipsec, rx, family, &attr);

	err = ipsec_rx_status_pass_dest_get(ipsec, rx, &attr, &dest[0]);
	if (err)
		return err;

	ft = ipsec_ft_create(attr.ns, attr.status_level, attr.prio, 3, 0);
	if (IS_ERR(ft)) {
		err = PTR_ERR(ft);
		goto err_fs_ft_status;
	}
	rx->ft.status = ft;

	dest[1].type = MLX5_FLOW_DESTINATION_TYPE_COUNTER;
	dest[1].counter_id = mlx5_fc_id(rx->fc->cnt);
	err = mlx5_ipsec_rx_status_create(ipsec, rx, dest);
	if (err)
		goto err_add;

	/* Create FT */
	if (mlx5_ipsec_device_caps(mdev) & MLX5_IPSEC_CAP_TUNNEL)
		rx->allow_tunnel_mode = mlx5_eswitch_block_encap(mdev);
	if (rx->allow_tunnel_mode)
		flags = MLX5_FLOW_TABLE_TUNNEL_EN_REFORMAT;
	ft = ipsec_ft_create(attr.ns, attr.sa_level, attr.prio, 2, flags);
	if (IS_ERR(ft)) {
		err = PTR_ERR(ft);
		goto err_fs_ft;
	}
	rx->ft.sa = ft;

	err = ipsec_miss_create(mdev, rx->ft.sa, &rx->sa, dest);
	if (err)
		goto err_fs;

	if (mlx5_ipsec_device_caps(mdev) & MLX5_IPSEC_CAP_PRIO) {
		rx->chains = ipsec_chains_create(mdev, rx->ft.sa,
						 attr.chains_ns,
						 attr.prio,
						 attr.pol_level,
						 &rx->ft.pol);
		if (IS_ERR(rx->chains)) {
			err = PTR_ERR(rx->chains);
			goto err_pol_ft;
		}

		goto connect;
	}

	ft = ipsec_ft_create(attr.ns, attr.pol_level, attr.prio, 2, 0);
	if (IS_ERR(ft)) {
		err = PTR_ERR(ft);
		goto err_pol_ft;
	}
	rx->ft.pol = ft;
	memset(dest, 0x00, 2 * sizeof(*dest));
	dest[0].type = MLX5_FLOW_DESTINATION_TYPE_FLOW_TABLE;
	dest[0].ft = rx->ft.sa;
	err = ipsec_miss_create(mdev, rx->ft.pol, &rx->pol, dest);
	if (err)
		goto err_pol_miss;

connect:
	/* connect */
	if (rx != ipsec->rx_esw)
		ipsec_rx_ft_connect(ipsec, rx, &attr);
	return 0;

err_pol_miss:
	mlx5_destroy_flow_table(rx->ft.pol);
err_pol_ft:
	mlx5_del_flow_rules(rx->sa.rule);
	mlx5_destroy_flow_group(rx->sa.group);
err_fs:
	mlx5_destroy_flow_table(rx->ft.sa);
err_fs_ft:
	if (rx->allow_tunnel_mode)
		mlx5_eswitch_unblock_encap(mdev);
	mlx5_ipsec_rx_status_destroy(ipsec, rx);
err_add:
	mlx5_destroy_flow_table(rx->ft.status);
err_fs_ft_status:
	mlx5_ipsec_fs_roce_rx_destroy(ipsec->roce, family, mdev);
	return err;
}
