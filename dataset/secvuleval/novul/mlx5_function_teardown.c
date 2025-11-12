static int mlx5_function_teardown(struct mlx5_core_dev *dev, bool boot)
{
	int err = mlx5_function_close(dev);

	if (!err)
		mlx5_function_disable(dev, boot);
	else
		mlx5_stop_health_poll(dev, boot);

	return err;
}
