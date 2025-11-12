void sysfb_disable(void)
{
	mutex_lock(&disable_lock);
	sysfb_unregister();
	disabled = true;
	mutex_unlock(&disable_lock);
}
