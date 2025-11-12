pretty_print_packet(netdissect_options *ndo, const struct pcap_pkthdr *h,
		    const u_char *sp, u_int packets_captured)
{
	u_int hdrlen = 0;
	int invalid_header = 0;

	if (ndo->ndo_print_sampling && packets_captured % ndo->ndo_print_sampling != 0)
		return;

#ifdef ENABLE_INSTRUMENT_FUNCTIONS
	if (pretty_print_packet_level == -1)
		pretty_print_packet_level = profile_func_level;
#endif

	if (ndo->ndo_packet_number)
		ND_PRINT("%5u  ", packets_captured);

	if (ndo->ndo_lengths)
		ND_PRINT("caplen %u len %u ", h->caplen, h->len);

	/* Sanity checks on packet length / capture length */
	if (h->caplen == 0) {
		invalid_header = 1;
		ND_PRINT("[Invalid header: caplen==0");
	}
	if (h->len == 0) {
		if (!invalid_header) {
			invalid_header = 1;
			ND_PRINT("[Invalid header:");
		} else
			ND_PRINT(",");
		ND_PRINT(" len==0");
	} else if (h->len < h->caplen) {
		if (!invalid_header) {
			invalid_header = 1;
			ND_PRINT("[Invalid header:");
		} else
			ND_PRINT(",");
		ND_PRINT(" len(%u) < caplen(%u)", h->len, h->caplen);
	}
	if (h->caplen > MAXIMUM_SNAPLEN) {
		if (!invalid_header) {
			invalid_header = 1;
			ND_PRINT("[Invalid header:");
		} else
			ND_PRINT(",");
		ND_PRINT(" caplen(%u) > %u", h->caplen, MAXIMUM_SNAPLEN);
	}
	if (h->len > MAXIMUM_SNAPLEN) {
		if (!invalid_header) {
			invalid_header = 1;
			ND_PRINT("[Invalid header:");
		} else
			ND_PRINT(",");
		ND_PRINT(" len(%u) > %u", h->len, MAXIMUM_SNAPLEN);
	}
	if (invalid_header) {
		ND_PRINT("]\n");
		return;
	}

	/*
	 * At this point:
	 *   capture length != 0,
	 *   packet length != 0,
	 *   capture length <= MAXIMUM_SNAPLEN,
	 *   packet length <= MAXIMUM_SNAPLEN,
	 *   packet length >= capture length.
	 *
	 * Currently, there is no D-Bus printer, thus no need for
	 * bigger lengths.
	 */

	/*
	 * The header /usr/include/pcap/pcap.h in OpenBSD declares h->ts as
	 * struct bpf_timeval, not struct timeval. The former comes from
	 * /usr/include/net/bpf.h and uses 32-bit unsigned types instead of
	 * the types used in struct timeval.
	 */
	struct timeval tvbuf;
	tvbuf.tv_sec = h->ts.tv_sec;
	tvbuf.tv_usec = h->ts.tv_usec;
	ts_print(ndo, &tvbuf);

	/*
	 * Printers must check that they're not walking off the end of
	 * the packet.
	 * Rather than pass it all the way down, we set this member
	 * of the netdissect_options structure.
	 */
	ndo->ndo_snapend = sp + h->caplen;
	ndo->ndo_packetp = sp;

	ndo->ndo_protocol = "";
	ndo->ndo_ll_hdr_len = 0;
	switch (setjmp(ndo->ndo_early_end)) {
	case 0:
		/* Print the packet. */
		(ndo->ndo_if_printer)(ndo, h, sp);
		break;
	case ND_TRUNCATED:
		/* A printer quit because the packet was truncated; report it */
		nd_print_trunc(ndo);
		/* Print the full packet */
		ndo->ndo_ll_hdr_len = 0;
#ifdef ENABLE_INSTRUMENT_FUNCTIONS
		/* truncation => reassignment */
		profile_func_level = pretty_print_packet_level;
#endif
		break;
	}
	hdrlen = ndo->ndo_ll_hdr_len;

	/*
	 * Empty the stack of packet information, freeing all pushed buffers;
	 * if we got here by a printer quitting, we need to release anything
	 * that didn't get released because we longjmped out of the code
	 * before it popped the packet information.
	 */
	nd_pop_all_packet_info(ndo);

	/*
	 * Restore the originals snapend and packetp, as a printer
	 * might have changed them.
	 *
	 * XXX - nd_pop_all_packet_info() should have restored the
	 * original values, but, just in case....
	 */
	ndo->ndo_snapend = sp + h->caplen;
	ndo->ndo_packetp = sp;
	if (ndo->ndo_Xflag) {
		/*
		 * Print the raw packet data in hex and ASCII.
		 */
		if (ndo->ndo_Xflag > 1) {
			/*
			 * Include the link-layer header.
			 */
			hex_and_ascii_print(ndo, "\n\t", sp, h->caplen);
		} else {
			/*
			 * Don't include the link-layer header - and if
			 * we have nothing past the link-layer header,
			 * print nothing.
			 */
			if (h->caplen > hdrlen)
				hex_and_ascii_print(ndo, "\n\t", sp + hdrlen,
						    h->caplen - hdrlen);
		}
	} else if (ndo->ndo_xflag) {
		/*
		 * Print the raw packet data in hex.
		 */
		if (ndo->ndo_xflag > 1) {
			/*
			 * Include the link-layer header.
			 */
			hex_print(ndo, "\n\t", sp, h->caplen);
		} else {
			/*
			 * Don't include the link-layer header - and if
			 * we have nothing past the link-layer header,
			 * print nothing.
			 */
			if (h->caplen > hdrlen)
				hex_print(ndo, "\n\t", sp + hdrlen,
					  h->caplen - hdrlen);
		}
	} else if (ndo->ndo_Aflag) {
		/*
		 * Print the raw packet data in ASCII.
		 */
		if (ndo->ndo_Aflag > 1) {
			/*
			 * Include the link-layer header.
			 */
			ascii_print(ndo, sp, h->caplen);
		} else {
			/*
			 * Don't include the link-layer header - and if
			 * we have nothing past the link-layer header,
			 * print nothing.
			 */
			if (h->caplen > hdrlen)
				ascii_print(ndo, sp + hdrlen, h->caplen - hdrlen);
		}
	}

	ND_PRINT("\n");
	nd_free_all(ndo);
}