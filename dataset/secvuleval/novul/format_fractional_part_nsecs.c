format_fractional_part_nsecs(gchar *buf, size_t buflen, guint32 nsecs, const char *decimal_point, int precision)
{
	gchar *ptr;
	size_t remaining;
	int num_bytes;
	gsize decimal_point_len;
	guint32 frac_part;
	gint8 num_buf[CHARS_NANOSECONDS];
	gint8 *num_end = &num_buf[CHARS_NANOSECONDS];
	gint8 *num_ptr;
	size_t num_len;

	ws_assert(precision != 0);

	if (buflen == 0) {
		/*
		 * No room in the buffer for anything, including
		 * a terminating '\0'.
		 */
		return 0;
	}

	/*
	 * If the fractional part is >= 1, don't show it as a
	 * fractional part.
	 */
	if (nsecs >= 1000000000U) {
		num_bytes = snprintf(buf, buflen, "%s(%u nanoseconds)",
		    decimal_point, nsecs);
		if ((unsigned int)num_bytes >= buflen) {
			/*
			 * That filled up or would have overflowed
			 * the buffer.  Nothing more to do; return
			 * the remaining space in the buffer, minus
			 * one byte for the terminating '\0',* as
			 * that's the number of bytes we copied.
			 */
			return (int)(buflen - 1);
		}
		return num_bytes;
	}

	ptr = buf;
	remaining = buflen;
	num_bytes = 0;

	/*
	 * Copy the decimal point.
	 */
	decimal_point_len = g_strlcpy(buf, decimal_point, buflen);
	if (decimal_point_len >= buflen) {
		/*
		 * The decimal point didn't fit in the buffer
		 * and was truncated.  Nothing more to do;
		 * return the remaining space in the buffer,
		 * minus one byte for the terminating '\0',
		 * as that's the number of bytes we copied.
		 */
		return (int)(buflen - 1);
	}
	ptr += decimal_point_len;
	remaining -= decimal_point_len;
	num_bytes += decimal_point_len;

	/*
	 * Fill in num_buf with the nanoseconds value, padded with
	 * leading zeroes, to the specified precision.
	 *
	 * We scale the fractional part in advance, as that just
	 * takes one division by a constant (which may be
	 * optimized to a faster multiplication by a constant)
	 * and gets rid of some divisions and remainders by 100
	 * done to generate the digits.
	 *
	 * We pass preciions as the last argument to
	 * uint_to_str_back_len(), as that might mean that
	 * all of the cases end up using common code to
	 * do part of the call to uint_to_str_back_len().
	 */
	switch (precision) {

	case 1:
		/*
		 * Scale down to units of 1/10 second.
		 */
		frac_part = nsecs / 100000000U;
		break;

	case 2:
		/*
		 * Scale down to units of 1/100 second.
		 */
		frac_part = nsecs / 10000000U;
		break;

	case 3:
		/*
		 * Scale down to units of 1/1000 second.
		 */
		frac_part = nsecs / 1000000U;
		break;

	case 4:
		/*
		 * Scale down to units of 1/10000 second.
		 */
		frac_part = nsecs / 100000U;
		break;

	case 5:
		/*
		 * Scale down to units of 1/100000 second.
		 */
		frac_part = nsecs / 10000U;
		break;

	case 6:
		/*
		 * Scale down to units of 1/1000000 second.
		 */
		frac_part = nsecs / 1000U;
		break;

	case 7:
		/*
		 * Scale down to units of 1/10000000 second.
		 */
		frac_part = nsecs / 100U;
		break;

	case 8:
		/*
		 * Scale down to units of 1/100000000 second.
		 */
		frac_part = nsecs / 10U;
		break;

	case 9:
		/*
		 * We're already in units of 1/1000000000 second.
		 */
		frac_part = nsecs;
		break;

	default:
		ws_assert_not_reached();
		break;
	}

	num_ptr = uint_to_str_back_len(num_end, frac_part, precision);

	/*
	 * The length of the string that we want to copy to the buffer
	 * is the minimum of:
	 *
	 *    the length of the digit string;
	 *    the remaining space in the buffer, minus 1 for the
	 *      terminating '\0'.
	 */
	num_len = MIN((size_t)(num_end - num_ptr), remaining - 1);
	if (num_len == 0) {
		/*
		 * Not enough room to copy anything.
		 * Return the number of bytes we've generated.
		 */
		return num_bytes;
	}

	/*
	 * Copy over the fractional part.
	 */
	memcpy(ptr, num_ptr, num_len);
	ptr += num_len;
	num_bytes += num_len;

	/*
	 * '\0'-terminate it.
	 */
	*ptr = '\0';
	return num_bytes;
}