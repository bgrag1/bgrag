format_nstime_as_iso8601(gchar *buf, size_t buflen, const nstime_t *ns,
    char *decimal_point, gboolean local, int precision)
{
	struct tm tm, *tmp;
	gchar *ptr;
	size_t remaining;
	int num_bytes;

	if (local)
		tmp = ws_localtime_r(&ns->secs, &tm);
	else
		tmp = ws_gmtime_r(&ns->secs, &tm);
	if (tmp == NULL) {
		snprintf(buf, buflen, "Not representable");
		return;
	}
	ptr = buf;
	remaining = buflen;
	num_bytes = snprintf(ptr, remaining,
	    "%04d-%02d-%02d %02d:%02d:%02d",
	    tmp->tm_year + 1900,
	    tmp->tm_mon + 1,
	    tmp->tm_mday,
	    tmp->tm_hour,
	    tmp->tm_min,
	    tmp->tm_sec);
	if (num_bytes < 0) {
		/*
		 * That got an error.
		 * Not much else we can do.
		 */
		snprintf(buf, buflen, "snprintf() failed");
		return;
	}
	if ((unsigned int)num_bytes >= remaining) {
		/*
		 * That filled up or would have overflowed the buffer.
		 * Nothing more we can do.
		 */
		return;
	}
	ptr += num_bytes;
	remaining -= num_bytes;

	if (precision != 0) {
		/*
		 * Append the fractional part.
		 * Get the nsecs as a 32-bit unsigned value, as it should
		 * never be negative, so we treat it as unsigned.
		 */
		format_fractional_part_nsecs(ptr, remaining, (guint32)ns->nsecs, decimal_point, precision);
	}
}