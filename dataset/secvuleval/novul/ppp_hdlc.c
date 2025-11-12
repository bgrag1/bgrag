ppp_hdlc(netdissect_options *ndo,
         const u_char *p, u_int length)
{
	u_int caplen = ND_BYTES_AVAILABLE_AFTER(p);
	u_char *b, *t, c;
	const u_char *s;
	u_int i, proto;

	if (caplen == 0)
		return;

        if (length == 0)
                return;

	b = (u_char *)malloc(caplen);
	if (b == NULL) {
		(*ndo->ndo_error)(ndo, S_ERR_ND_MEM_ALLOC,
			"%s: malloc", __func__);
	}

	/*
	 * Unescape all the data into a temporary, private, buffer.
	 * Do this so that we don't overwrite the original packet
	 * contents.
	 */
	for (s = p, t = b, i = caplen; i != 0; i--) {
		c = GET_U_1(s);
		s++;
		if (c == 0x7d) {
			if (i <= 1)
				break;
			i--;
			c = GET_U_1(s) ^ 0x20;
			s++;
		}
		*t++ = c;
	}

	/*
	 * Switch to the output buffer for dissection, and save it
	 * on the buffer stack so it can be freed; our caller must
	 * pop it when done.
	 */
	if (!nd_push_buffer(ndo, b, b, (u_int)(t - b))) {
		free(b);
		(*ndo->ndo_error)(ndo, S_ERR_ND_MEM_ALLOC,
			"%s: can't push buffer on buffer stack", __func__);
	}
	length = ND_BYTES_AVAILABLE_AFTER(b);

        /* now lets guess about the payload codepoint format */
        if (length < 1)
                goto trunc;
        proto = GET_U_1(b); /* start with a one-octet codepoint guess */

        switch (proto) {
        case PPP_IP:
		ip_print(ndo, b + 1, length - 1);
		goto cleanup;
        case PPP_IPV6:
		ip6_print(ndo, b + 1, length - 1);
		goto cleanup;
        default: /* no luck - try next guess */
		break;
        }

        if (length < 2)
                goto trunc;
        proto = GET_BE_U_2(b); /* next guess - load two octets */

        switch (proto) {
        case (PPP_ADDRESS << 8 | PPP_CONTROL): /* looks like a PPP frame */
            if (length < 4)
                goto trunc;
            proto = GET_BE_U_2(b + 2); /* load the PPP proto-id */
            if ((proto & 0xff00) == 0x7e00)
                ND_PRINT("(protocol 0x%04x invalid)", proto);
            else
                handle_ppp(ndo, proto, b + 4, length - 4);
            break;
        default: /* last guess - proto must be a PPP proto-id */
            if ((proto & 0xff00) == 0x7e00)
                ND_PRINT("(protocol 0x%04x invalid)", proto);
            else
                handle_ppp(ndo, proto, b + 2, length - 2);
            break;
        }

cleanup:
	nd_pop_packet_info(ndo);
        return;

trunc:
	nd_pop_packet_info(ndo);
	nd_print_trunc(ndo);
}