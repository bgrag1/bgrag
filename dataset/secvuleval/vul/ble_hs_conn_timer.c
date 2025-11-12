ble_hs_conn_timer(void)
{
    /* If there are no timeouts configured, then there is nothing to check. */
#if MYNEWT_VAL(BLE_L2CAP_RX_FRAG_TIMEOUT) == 0 && \
    BLE_HS_ATT_SVR_QUEUED_WRITE_TMO == 0

    return BLE_HS_FOREVER;
#endif

    struct ble_hs_conn *conn;
    ble_npl_time_t now;
    int32_t next_exp_in;
    int32_t time_diff;
    uint16_t conn_handle;

    for (;;) {
        conn_handle = BLE_HS_CONN_HANDLE_NONE;
        next_exp_in = BLE_HS_FOREVER;
        now = ble_npl_time_get();

        ble_hs_lock();

        /* This loop performs one of two tasks:
         * 1. Determine if any connections need to be terminated due to timeout.
         *    If so, break out of the loop and terminate the connection.  This
         *    function will need to be executed again.
         * 2. Otherwise, determine when the next timeout will occur.
         */
        SLIST_FOREACH(conn, &ble_hs_conns, bhc_next) {
            if (!(conn->bhc_flags & BLE_HS_CONN_F_TERMINATING)) {

#if MYNEWT_VAL(BLE_L2CAP_RX_FRAG_TIMEOUT) != 0
                /* Check each connection's rx fragment timer.  If too much time
                 * passes after a partial packet is received, the connection is
                 * terminated.
                 */
                if (conn->bhc_rx_chan != NULL) {
                    time_diff = conn->bhc_rx_timeout - now;

                    if (time_diff <= 0) {
                        /* ACL reassembly has timed out.  Remember the connection
                         * handle so it can be terminated after the mutex is
                         * unlocked.
                         */
                        conn_handle = conn->bhc_handle;
                        break;
                    }

                    /* Determine if this connection is the soonest to time out. */
                    if (time_diff < next_exp_in) {
                        next_exp_in = time_diff;
                    }
                }
#endif

#if BLE_HS_ATT_SVR_QUEUED_WRITE_TMO
                /* Check each connection's rx queued write timer.  If too much
                 * time passes after a prep write is received, the queue is
                 * cleared.
                 */
                time_diff = ble_att_svr_ticks_until_tmo(&conn->bhc_att_svr, now);
                if (time_diff <= 0) {
                    /* ACL reassembly has timed out.  Remember the connection
                     * handle so it can be terminated after the mutex is
                     * unlocked.
                     */
                    conn_handle = conn->bhc_handle;
                    break;
                }

                /* Determine if this connection is the soonest to time out. */
                if (time_diff < next_exp_in) {
                    next_exp_in = time_diff;
                }
#endif
            }
        }

        ble_hs_unlock();

        /* If a connection has timed out, terminate it.  We need to repeatedly
         * call this function again to determine when the next timeout is.
         */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
            continue;
        }

        return next_exp_in;
    }
}