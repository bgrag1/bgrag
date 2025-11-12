htp_status_t htp_connp_REQ_PROTOCOL(htp_connp_t *connp) {
    // Is this a short-style HTTP/0.9 request? If it is,
    // we will not want to parse request headers.
    if (connp->in_tx->is_protocol_0_9 == 0) {
        // Switch to request header parsing.
        connp->in_state = htp_connp_REQ_HEADERS;
        connp->in_tx->request_progress = HTP_REQUEST_HEADERS;
    } else {
        // Let's check if the protocol was simply missing
        int64_t pos = connp->in_current_read_offset;
        // Probe if data looks like a header line
        if (connp->in_current_len > connp->in_current_read_offset + HTTP09_MAX_JUNK_LEN) {
            htp_log(connp, HTP_LOG_MARK, HTP_LOG_WARNING, 0, "Request line: missing protocol");
            connp->in_tx->is_protocol_0_9 = 0;
            // Switch to request header parsing.
            connp->in_state = htp_connp_REQ_HEADERS;
            connp->in_tx->request_progress = HTP_REQUEST_HEADERS;
            return HTP_OK;
        }
        while (pos < connp->in_current_len) {
            if (!htp_is_space(connp->in_current_data[pos])) {
                htp_log(connp, HTP_LOG_MARK, HTP_LOG_WARNING, 0, "Request line: missing protocol");
                connp->in_tx->is_protocol_0_9 = 0;
                // Switch to request header parsing.
                connp->in_state = htp_connp_REQ_HEADERS;
                connp->in_tx->request_progress = HTP_REQUEST_HEADERS;
                return HTP_OK;
            }
            pos++;
        }
        // We're done with this request.
        connp->in_state = htp_connp_REQ_FINALIZE;
    }

    return HTP_OK;
}