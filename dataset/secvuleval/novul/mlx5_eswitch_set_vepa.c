int mlx5_eswitch_set_vepa(struct mlx5_eswitch *esw, u8 setting)
{
	int err = 0;

	if (!esw)
		return -EOPNOTSUPP;

	if (!mlx5_esw_allowed(esw))
		return -EPERM;

	mutex_lock(&esw->state_lock);
	if (esw->mode != MLX5_ESWITCH_LEGACY || !mlx5_esw_is_fdb_created(esw)) {
		err = -EOPNOTSUPP;
		goto out;
	}

	err = _mlx5_eswitch_set_vepa_locked(esw, setting);

out:
	mutex_unlock(&esw->state_lock);
	return err;
}
