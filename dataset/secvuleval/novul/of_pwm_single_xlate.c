struct pwm_device *
of_pwm_single_xlate(struct pwm_chip *chip, const struct of_phandle_args *args)
{
	struct pwm_device *pwm;

	if (chip->of_pwm_n_cells < 1)
		return ERR_PTR(-EINVAL);

	/* validate that one cell is specified, optionally with flags */
	if (args->args_count != 1 && args->args_count != 2)
		return ERR_PTR(-EINVAL);

	pwm = pwm_request_from_chip(chip, 0, NULL);
	if (IS_ERR(pwm))
		return pwm;

	pwm->args.period = args->args[0];
	pwm->args.polarity = PWM_POLARITY_NORMAL;

	if (args->args_count == 2 && args->args[1] & PWM_POLARITY_INVERTED)
		pwm->args.polarity = PWM_POLARITY_INVERSED;

	return pwm;
}
