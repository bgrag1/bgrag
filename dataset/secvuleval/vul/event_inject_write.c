static ssize_t
event_inject_write(struct file *filp, const char __user *ubuf, size_t cnt,
		   loff_t *ppos)
{
	struct trace_event_call *call;
	struct trace_event_file *file;
	int err = -ENODEV, size;
	void *entry = NULL;
	char *buf;

	if (cnt >= PAGE_SIZE)
		return -EINVAL;

	buf = memdup_user_nul(ubuf, cnt);
	if (IS_ERR(buf))
		return PTR_ERR(buf);
	strim(buf);

	mutex_lock(&event_mutex);
	file = event_file_data(filp);
	if (file) {
		call = file->event_call;
		size = parse_entry(buf, call, &entry);
		if (size < 0)
			err = size;
		else
			err = trace_inject_entry(file, entry, size);
	}
	mutex_unlock(&event_mutex);

	kfree(entry);
	kfree(buf);

	if (err < 0)
		return err;

	*ppos += err;
	return cnt;
}
