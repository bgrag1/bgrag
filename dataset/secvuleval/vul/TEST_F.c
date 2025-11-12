TEST_F(ConnectionParsing, Http_0_9_Multiple) {
    int rc = test_run(home, "23-http09-multiple.t", cfg, &connp);
    ASSERT_GE(rc, 0);

    ASSERT_EQ(1, htp_list_size(connp->conn->transactions));
    ASSERT_TRUE(connp->conn->flags & HTP_CONN_HTTP_0_9_EXTRA);

    htp_tx_t *tx = (htp_tx_t *) htp_list_get(connp->conn->transactions, 0);
    ASSERT_TRUE(tx != NULL);
}