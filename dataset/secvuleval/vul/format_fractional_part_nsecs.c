format_fractional_part_nsecs(gchar *buf, size_t buflen, guint32 nsecs, const char *decimal_point, int precision)
{
	gsize decimal_point_len;
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
		return;
	}

	/*
	 * Copy the decimal point.
	 */
	decimal_point_len = g_strlcpy(buf, decimal_point, buflen);
	if (decimal_point_len >= buflen) {
		/*
		 * The decimal point didn't fit in the buffer
		 * and was truncated.  Nothing more to do.
		 */
		return;
	}
	buf += decimal_point_len;
	buflen -= decimal_point_len;

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
		num_ptr = uint_to_str_back_len(num_end,
		    nsecs / 100000000, precision);
		break;

	case 2:
		/*
		 * Scale down to units of 1/100 second.
		 */
		num_ptr = uint_to_str_back_len(num_end,
		    nsecs / 10000000, precision);
		break;

	case 3:
		/*
		 * Scale down to units of 1/1000 second.
		 */
		num_ptr = uint_to_str_back_len(num_end,
		    nsecs / 1000000, precision);
		break;

	case 4:
		/*
		 * Scale down to units of 1/10000 second.
		 */
		num_ptr = uint_to_str_back_len(num_end,
		    nsecs / 100000, precision);
		break;

	case 5:
		/*
		 * Scale down to units of 1/100000 second.
		 */
		num_ptr = uint_to_str_back_len(num_end,
		    nsecs / 10000, precision);
		break;

	case 6:
		/*
		 * Scale down to units of 1/1000000 second.
		 */
		num_ptr = uint_to_str_back_len(num_end,
		    nsecs / 1000, precision);
		break;

	case 7:
		/*
		 * Scale down to units of 1/10000000 second.
		 */
		num_ptr = uint_to_str_back_len(num_end,
		    nsecs / 100, precision);
		break;

	case 8:
		/*
		 * Scale down to units of 1/100000000 second.
		 */
		num_ptr = uint_to_str_back_len(num_end,
		    nsecs / 10, precision);
		break;

	case 9:
		/*
		 * We're already in units of 1/1000000000 second.
		 */
		num_ptr = uint_to_str_back_len(num_end, nsecs,
		    precision);
		break;

	default:
		ws_assert_not_reached();
		break;
	}

	/*
	 * The length of the string that we want to copy to the buffer
	 * is the minimum of:
	 *
	 *    the length of the digit string;
	 *    the size of the buffer, minus 1 for the terminating
	 *      '\0'.
	 */
	num_len = MIN((size_t)(num_end - num_ptr), buflen - 1);
	if (num_len == 0) {
		/*
		 * Not enough room to copy anything.
		 */
		return;
	}

	/*
	 * Copy over the fractional part.
	 */
	memcpy(buf, num_ptr, num_len);

	/*
	 * '\0'-terminate it.
	 */
	*(buf + num_len) = '\0';
}