static int ima_eventname_init_common(struct ima_event_data *event_data,
				     struct ima_field_data *field_data,
				     bool size_limit)
{
	const char *cur_filename = NULL;
	struct name_snapshot filename;
	u32 cur_filename_len = 0;
	bool snapshot = false;
	int ret;

	BUG_ON(event_data->filename == NULL && event_data->file == NULL);

	if (event_data->filename) {
		cur_filename = event_data->filename;
		cur_filename_len = strlen(event_data->filename);

		if (!size_limit || cur_filename_len <= IMA_EVENT_NAME_LEN_MAX)
			goto out;
	}

	if (event_data->file) {
		take_dentry_name_snapshot(&filename,
					  event_data->file->f_path.dentry);
		snapshot = true;
		cur_filename = filename.name.name;
		cur_filename_len = strlen(cur_filename);
	} else
		/*
		 * Truncate filename if the latter is too long and
		 * the file descriptor is not available.
		 */
		cur_filename_len = IMA_EVENT_NAME_LEN_MAX;
out:
	ret = ima_write_template_field_data(cur_filename, cur_filename_len,
					    DATA_FMT_STRING, field_data);

	if (snapshot)
		release_dentry_name_snapshot(&filename);

	return ret;
}
