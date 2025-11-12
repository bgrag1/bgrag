static void process_request(struct connection *conn) {
    num_requests++;

    if (!parse_request(conn)) {
        default_reply(conn, 400, "Bad Request",
            "You sent a request that the server couldn't understand.");
    }
    else if (is_https_redirect(conn)) {
        redirect_https(conn);
    }
    /* fail if: (auth_enabled) AND (client supplied invalid credentials) */
    else if (auth_key != NULL &&
            (conn->authorization == NULL ||
             !password_equal(conn->authorization, auth_key))) {
        default_reply(conn, 401, "Unauthorized",
            "Access denied due to invalid credentials.");
    }
    else if (strcmp(conn->method, "GET") == 0) {
        process_get(conn);
    }
    else if (strcmp(conn->method, "HEAD") == 0) {
        process_get(conn);
        conn->header_only = 1;
    }
    else {
        default_reply(conn, 501, "Not Implemented",
                      "The method you specified is not implemented.");
    }

    /* advance state */
    conn->state = SEND_HEADER;

    /* request not needed anymore */
    free(conn->request);
    conn->request = NULL; /* important: don't free it again later */
}