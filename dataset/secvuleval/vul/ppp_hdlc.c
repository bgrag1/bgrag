ppp_hdlc(netdissect_options *ndo,
         const u_char *p, u_int length)
{
	u_int caplen = ND_BYTES_AVAILABLE_AFTER(p);
	u_char *b, *t, c;
	const u_char *s;
	u_int i, proto;
	const void *sb, *se;

	if (caplen == 0)
		return;

        if (length == 0)
                return;

	b = (u_char *)nd_malloc(ndo, caplen);
	if (b == NULL)
		return;

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
	 * Change the end pointer, so bounds checks work.
	 * Change the pointer to packet data to help debugging.
	 */
	sb = ndo->ndo_packetp;
	se = ndo->ndo_snapend;
	ndo->ndo_packetp = b;
	ndo->ndo_snapend = t;
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
	ndo->ndo_packetp = sb;
	ndo->ndo_snapend = se;
        return;

trunc:
	ndo->ndo_packetp = sb;
	ndo->ndo_snapend = se;
	nd_print_trunc(ndo);
}