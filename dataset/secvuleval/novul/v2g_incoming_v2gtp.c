static int v2g_incoming_v2gtp(struct v2g_connection* conn) {
    int rv;

    /* read and process header */
    rv = connection_read(conn, conn->buffer, V2GTP_HEADER_LENGTH);
    if (rv < 0) {
        dlog(DLOG_LEVEL_ERROR, "connection_read(header) failed: %s",
             (rv == -1) ? strerror(errno) : "connection terminated");
        return -1;
    }
    /* peer closed connection */
    if (rv == 0)
        return 1;
    if (rv != V2GTP_HEADER_LENGTH) {
        dlog(DLOG_LEVEL_ERROR, "connection_read(header) too short: expected %d, got %d", V2GTP_HEADER_LENGTH, rv);
        return -1;
    }

    rv = read_v2gtpHeader(conn->buffer, &conn->payload_len);
    if (rv == -1) {
        dlog(DLOG_LEVEL_ERROR, "Invalid v2gtp header");
        return -1;
    }

    if (conn->payload_len >= UINT32_MAX - V2GTP_HEADER_LENGTH) {
        dlog(DLOG_LEVEL_ERROR, "Prevent integer overflow - payload too long: have %d, would need %u",
             DEFAULT_BUFFER_SIZE, conn->payload_len);
        return -1;
    }

    if (conn->payload_len + V2GTP_HEADER_LENGTH > DEFAULT_BUFFER_SIZE) {
        dlog(DLOG_LEVEL_ERROR, "payload too long: have %d, would need %u", DEFAULT_BUFFER_SIZE,
             conn->payload_len + V2GTP_HEADER_LENGTH);

        /* we have no way to flush/discard remaining unread data from the socket without reading it in chunks,
         * but this opens the chance to bind us in a "endless" read loop; so to protect us, simply close the connection
         */

        return -1;
    }
    /* read request */
    rv = connection_read(conn, &conn->buffer[V2GTP_HEADER_LENGTH], conn->payload_len);
    if (rv < 0) {
        dlog(DLOG_LEVEL_ERROR, "connection_read(payload) failed: %s",
             (rv == -1) ? strerror(errno) : "connection terminated");
        return -1;
    }
    if (rv != conn->payload_len) {
        dlog(DLOG_LEVEL_ERROR, "connection_read(payload) too short: expected %d, got %d", conn->payload_len, rv);
        return -1;
    }
    /* adjust buffer pos to decode request */
    conn->buffer_pos = V2GTP_HEADER_LENGTH;
    conn->stream.size = conn->payload_len + V2GTP_HEADER_LENGTH;

    return 0;
}