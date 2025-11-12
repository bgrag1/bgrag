static int cmd_comp_notifier(struct notifier_block *nb,
			     unsigned long type, void *data)
{
	struct mlx5_core_dev *dev;
	struct mlx5_cmd *cmd;
	struct mlx5_eqe *eqe;

	cmd = mlx5_nb_cof(nb, struct mlx5_cmd, nb);
	dev = container_of(cmd, struct mlx5_core_dev, cmd);
	eqe = data;

	if (dev->state == MLX5_DEVICE_STATE_INTERNAL_ERROR)
		return NOTIFY_DONE;

	mlx5_cmd_comp_handler(dev, be32_to_cpu(eqe->data.cmd.vector), false);

	return NOTIFY_OK;
}
