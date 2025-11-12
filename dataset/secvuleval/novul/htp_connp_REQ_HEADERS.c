htp_status_t htp_connp_REQ_HEADERS(htp_connp_t *connp) {
    for (;;) {
        if (connp->in_status == HTP_STREAM_CLOSED) {
            // Parse previous header, if any.
            if (connp->in_header != NULL) {
                if (connp->cfg->process_request_header(connp, bstr_ptr(connp->in_header),
                                                       bstr_len(connp->in_header)) != HTP_OK)
                    return HTP_ERROR;
                bstr_free(connp->in_header);
                connp->in_header = NULL;
            }

            htp_connp_req_clear_buffer(connp);

            connp->in_tx->request_progress = HTP_REQUEST_TRAILER;

            // We've seen all the request headers.
            return htp_tx_state_request_headers(connp->in_tx);
        }
        IN_COPY_BYTE_OR_RETURN(connp);

        // Have we reached the end of the line?
        if (connp->in_next_byte == LF) {
            unsigned char *data;
            size_t len;

            if (htp_connp_req_consolidate_data(connp, &data, &len) != HTP_OK) {
                return HTP_ERROR;
            }

            #ifdef HTP_DEBUG
            fprint_raw_data(stderr, __func__, data, len);
            #endif           

            // Should we terminate headers?
            if (htp_connp_is_line_terminator(connp, data, len, 0)) {
                // Parse previous header, if any.
                if (connp->in_header != NULL) {
                    if (connp->cfg->process_request_header(connp, bstr_ptr(connp->in_header),
                            bstr_len(connp->in_header)) != HTP_OK) return HTP_ERROR;

                    bstr_free(connp->in_header);
                    connp->in_header = NULL;
                }

                htp_connp_req_clear_buffer(connp);

                // We've seen all the request headers.
                return htp_tx_state_request_headers(connp->in_tx);
            }

            htp_chomp(data, &len);

            // Check for header folding.
            if (htp_connp_is_line_folded(data, len) == 0) {
                // New header line.

                // Parse previous header, if any.
                if (connp->in_header != NULL) {
                    if (connp->cfg->process_request_header(connp, bstr_ptr(connp->in_header),
                            bstr_len(connp->in_header)) != HTP_OK) return HTP_ERROR;

                    bstr_free(connp->in_header);
                    connp->in_header = NULL;
                }

                IN_PEEK_NEXT(connp);

                if (connp->in_next_byte != -1 && htp_is_folding_char(connp->in_next_byte) == 0) {
                    // Because we know this header is not folded, we can process the buffer straight away.
                    if (connp->cfg->process_request_header(connp, data, len) != HTP_OK) return HTP_ERROR;
                } else {
                    // Keep the partial header data for parsing later.
                    connp->in_header = bstr_dup_mem(data, len);
                    if (connp->in_header == NULL) return HTP_ERROR;
                }
            } else {
                // Folding; check that there's a previous header line to add to.
                if (connp->in_header == NULL) {
                    // Invalid folding.

                    // Warn only once per transaction.
                    if (!(connp->in_tx->flags & HTP_INVALID_FOLDING)) {
                        connp->in_tx->flags |= HTP_INVALID_FOLDING;
                        htp_log(connp, HTP_LOG_MARK, HTP_LOG_WARNING, 0, "Invalid request field folding");
                    }

                    // Keep the header data for parsing later.
                    size_t trim = 0;
                    while(trim < len) {
                        if (!htp_is_folding_char(data[trim])) {
                            break;
                        }
                        trim++;
                    }
                    connp->in_header = bstr_dup_mem(data + trim, len - trim);
                    if (connp->in_header == NULL) return HTP_ERROR;
                } else {
                    // Add to the existing header.
                    if (bstr_len(connp->in_header) < HTP_MAX_HEADER_FOLDED) {
                        bstr *new_in_header = bstr_add_mem(connp->in_header, data, len);
                        if (new_in_header == NULL) return HTP_ERROR;
                        connp->in_header = new_in_header;
                    } else {
                        htp_log(connp, HTP_LOG_MARK, HTP_LOG_WARNING, 0, "Request field length exceeds folded maximum");
                    }
                }
            }

            htp_connp_req_clear_buffer(connp);
        }
    }

    return HTP_ERROR;
}