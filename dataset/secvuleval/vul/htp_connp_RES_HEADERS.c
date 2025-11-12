htp_status_t htp_connp_RES_HEADERS(htp_connp_t *connp) {
    int endwithcr;
    int lfcrending = 0;

    for (;;) {
        if (connp->out_status == HTP_STREAM_CLOSED) {
            // Finalize sending raw trailer data.
            htp_status_t rc = htp_connp_res_receiver_finalize_clear(connp);
            if (rc != HTP_OK) return rc;

            // Run hook response_TRAILER.
            rc = htp_hook_run_all(connp->cfg->hook_response_trailer, connp->out_tx);
            if (rc != HTP_OK) return rc;

            connp->out_state = htp_connp_RES_FINALIZE;
            return HTP_OK;
        }
        OUT_COPY_BYTE_OR_RETURN(connp);

        // Have we reached the end of the line?
        if (connp->out_next_byte != LF && connp->out_next_byte != CR) {
            lfcrending = 0;
        } else {
            endwithcr = 0;
            if (connp->out_next_byte == CR) {
                OUT_PEEK_NEXT(connp);
                if (connp->out_next_byte == -1) {
                    return HTP_DATA_BUFFER;
                } else if (connp->out_next_byte == LF) {
                    OUT_COPY_BYTE_OR_RETURN(connp);
                    if (lfcrending) {
                        // Handling LFCRCRLFCRLF
                        // These 6 characters mean only 2 end of lines
                        OUT_PEEK_NEXT(connp);
                        if (connp->out_next_byte == CR) {
                            OUT_COPY_BYTE_OR_RETURN(connp);
                            connp->out_current_consume_offset++;
                            OUT_PEEK_NEXT(connp);
                            if (connp->out_next_byte == LF) {
                                OUT_COPY_BYTE_OR_RETURN(connp);
                                connp->out_current_consume_offset++;
                                htp_log(connp, HTP_LOG_MARK, HTP_LOG_WARNING, 0,
                                        "Weird response end of lines mix");
                            }
                        }
                    }
                } else if (connp->out_next_byte == CR) {
                    continue;
                }
                lfcrending = 0;
                endwithcr = 1;
            } else {
                // connp->out_next_byte == LF
                OUT_PEEK_NEXT(connp);
                lfcrending = 0;
                if (connp->out_next_byte == CR) {
                    // hanldes LF-CR sequence as end of line
                    OUT_COPY_BYTE_OR_RETURN(connp);
                    lfcrending = 1;
                }
            }

            unsigned char *data;
            size_t len;

            if (htp_connp_res_consolidate_data(connp, &data, &len) != HTP_OK) {
                return HTP_ERROR;
            }

            // CRCRLF is not an empty line
            if (endwithcr && len < 2) {
                continue;
            }

            #ifdef HTP_DEBUG
            fprint_raw_data(stderr, __func__, data, len);
            #endif

            int next_no_lf = 0;
            if (connp->out_current_read_offset < connp->out_current_len &&
                connp->out_current_data[connp->out_current_read_offset] != LF) {
                next_no_lf = 1;
            }
            // Should we terminate headers?
            if (htp_connp_is_line_terminator(connp, data, len, next_no_lf)) {
                // Parse previous header, if any.
                if (connp->out_header != NULL) {
                    if (connp->cfg->process_response_header(connp, bstr_ptr(connp->out_header),
                            bstr_len(connp->out_header)) != HTP_OK) return HTP_ERROR;

                    bstr_free(connp->out_header);
                    connp->out_header = NULL;
                }

                htp_connp_res_clear_buffer(connp);

                // We've seen all response headers.
                if (connp->out_tx->response_progress == HTP_RESPONSE_HEADERS) {
                    // Response headers.

                    // The next step is to determine if this response has a body.
                    connp->out_state = htp_connp_RES_BODY_DETERMINE;
                } else {
                    // Response trailer.

                    // Finalize sending raw trailer data.
                    htp_status_t rc = htp_connp_res_receiver_finalize_clear(connp);
                    if (rc != HTP_OK) return rc;

                    // Run hook response_TRAILER.
                    rc = htp_hook_run_all(connp->cfg->hook_response_trailer, connp->out_tx);
                    if (rc != HTP_OK) return rc;

                    // The next step is to finalize this response.
                    connp->out_state = htp_connp_RES_FINALIZE;
                }

                return HTP_OK;
            }

            htp_chomp(data, &len);

            // Check for header folding.
            if (htp_connp_is_line_folded(data, len) == 0) {
                // New header line.

                // Parse previous header, if any.
                if (connp->out_header != NULL) {
                    if (connp->cfg->process_response_header(connp, bstr_ptr(connp->out_header),
                            bstr_len(connp->out_header)) != HTP_OK) return HTP_ERROR;

                    bstr_free(connp->out_header);
                    connp->out_header = NULL;
                }

                OUT_PEEK_NEXT(connp);

                if (htp_is_folding_char(connp->out_next_byte) == 0) {
                    // Because we know this header is not folded, we can process the buffer straight away.
                    if (connp->cfg->process_response_header(connp, data, len) != HTP_OK) return HTP_ERROR;
                } else {
                    // Keep the partial header data for parsing later.
                    connp->out_header = bstr_dup_mem(data, len);
                    if (connp->out_header == NULL) return HTP_ERROR;
                }
            } else {
                // Folding; check that there's a previous header line to add to.
                if (connp->out_header == NULL) {
                    // Invalid folding.

                    // Warn only once per transaction.
                    if (!(connp->out_tx->flags & HTP_INVALID_FOLDING)) {
                        connp->out_tx->flags |= HTP_INVALID_FOLDING;
                        htp_log(connp, HTP_LOG_MARK, HTP_LOG_WARNING, 0, "Invalid response field folding");
                    }

                    // Keep the header data for parsing later.
                    size_t trim = 0;
                    while(trim < len) {
                        if (!htp_is_folding_char(data[trim])) {
                            break;
                        }
                        trim++;
                    }
                    connp->out_header = bstr_dup_mem(data + trim, len - trim);
                    if (connp->out_header == NULL) return HTP_ERROR;
                } else {
                    size_t colon_pos = 0;
                    while ((colon_pos < len) && (data[colon_pos] != ':')) colon_pos++;

                    if (colon_pos < len &&
                        bstr_chr(connp->out_header, ':') >= 0 &&
                        connp->out_tx->response_protocol_number == HTP_PROTOCOL_1_1) {
                        // Warn only once per transaction.
                        if (!(connp->out_tx->flags & HTP_INVALID_FOLDING)) {
                            connp->out_tx->flags |= HTP_INVALID_FOLDING;
                            htp_log(connp, HTP_LOG_MARK, HTP_LOG_WARNING, 0, "Invalid response field folding");
                        }
                        if (connp->cfg->process_response_header(connp, bstr_ptr(connp->out_header),
                            bstr_len(connp->out_header)) != HTP_OK)
                            return HTP_ERROR;
                        bstr_free(connp->out_header);
                        connp->out_header = bstr_dup_mem(data+1, len-1);
                        if (connp->out_header == NULL)
                            return HTP_ERROR;
                    } else {
                        // Add to the existing header.
                        bstr *new_out_header = bstr_add_mem(connp->out_header, data, len);
                        if (new_out_header == NULL)
                            return HTP_ERROR;
                        connp->out_header = new_out_header;
                    }
                }
            }

            htp_connp_res_clear_buffer(connp);
        }
    }

    return HTP_ERROR;
}