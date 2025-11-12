static int _mlx5e_suspend(struct auxiliary_device *adev, bool pre_netdev_reg)
{
	struct mlx5e_dev *mlx5e_dev = auxiliary_get_drvdata(adev);
	struct mlx5e_priv *priv = mlx5e_dev->priv;
	struct net_device *netdev = priv->netdev;
	struct mlx5_core_dev *mdev = priv->mdev;
	struct mlx5_core_dev *pos;
	int i;

	if (!pre_netdev_reg && !netif_device_present(netdev)) {
		if (test_bit(MLX5E_STATE_DESTROYING, &priv->state))
			mlx5_sd_for_each_dev(i, mdev, pos)
				mlx5e_destroy_mdev_resources(pos);
		return -ENODEV;
	}

	mlx5e_detach_netdev(priv);
	mlx5_sd_for_each_dev(i, mdev, pos)
		mlx5e_destroy_mdev_resources(pos);

	return 0;
}
