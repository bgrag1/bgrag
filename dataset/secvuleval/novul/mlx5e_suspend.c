static int mlx5e_suspend(struct auxiliary_device *adev, pm_message_t state)
{
	struct mlx5_adev *edev = container_of(adev, struct mlx5_adev, adev);
	struct mlx5_core_dev *mdev = edev->mdev;
	struct auxiliary_device *actual_adev;
	int err = 0;

	actual_adev = mlx5_sd_get_adev(mdev, adev, edev->idx);
	if (actual_adev)
		err = _mlx5e_suspend(actual_adev, false);

	mlx5_sd_cleanup(mdev);
	return err;
}
