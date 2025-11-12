format_nstime_as_iso8601(gchar *buf, size_t buflen, const nstime_t *ns,
    char *decimal_point, gboolean local, int precision)
{
	struct tm tm, *tmp;
	gchar *ptr;
	size_t buf_remaining;
	int num_chars;

	if (local)
		tmp = ws_localtime_r(&ns->secs, &tm);
	else
		tmp = ws_gmtime_r(&ns->secs, &tm);
	if (tmp == NULL) {
		snprintf(buf, buflen, "Not representable");
		return;
	}
	ptr = buf;
	buf_remaining = buflen;
	num_chars = snprintf(ptr, buf_remaining,
	    "%04d-%02d-%02d %02d:%02d:%02d",
	    tmp->tm_year + 1900,
	    tmp->tm_mon + 1,
	    tmp->tm_mday,
	    tmp->tm_hour,
	    tmp->tm_min,
	    tmp->tm_sec);
	if (num_chars < 0) {
		/*
		 * Not much else we can do.
		 */
		snprintf(buf, buflen, "snprintf() failed");
		return;
	}
	if ((unsigned int)num_chars >= buf_remaining) {
		/*
		 * Either that got an error (num_chars < 0) or it
		 * filled up or would have overflowed the buffer
		 * (num_chars >= buf_remaining).
		 * Nothing more we can do.
		 */
		return;
	}
	ptr += num_chars;
	buf_remaining -= num_chars;

	if (precision == 0) {
		/*
		 * Seconds precision, so no nanosecond.
		 */
		return;
	}

	/*
	 * Append the fractional part.
	 */
	format_fractional_part_nsecs(ptr, buf_remaining, (guint32)ns->nsecs, decimal_point, precision);
}