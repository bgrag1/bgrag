static int
process_regular_packet (struct ietf_full_conn *conn,
                                        struct lsquic_packet_in *packet_in)
{
    struct conn_path *cpath;
    enum packnum_space pns;
    enum received_st st;
    enum dec_packin dec_packin;
    enum was_missing was_missing;
    int is_rechist_empty;
    unsigned char saved_path_id;
    int is_dcid_changed;

    if (HETY_RETRY == packet_in->pi_header_type)
        return process_retry_packet(conn, packet_in);

    CONN_STATS(in.packets, 1);

    pns = lsquic_hety2pns[ packet_in->pi_header_type ];
    if ((pns == PNS_INIT && (conn->ifc_flags & IFC_IGNORE_INIT))
                    || (pns == PNS_HSK  && (conn->ifc_flags & IFC_IGNORE_HSK)))
    {
        /* Don't bother decrypting */
        LSQ_DEBUG("ignore %s packet",
            pns == PNS_INIT ? "Initial" : "Handshake");
        EV_LOG_CONN_EVENT(LSQUIC_LOG_CONN_ID, "ignore %s packet",
                                                        lsquic_pns2str[pns]);
        return 0;
    }

    /* If a client receives packets from an unknown server address, the client
     * MUST discard these packets.
     *      [draft-ietf-quic-transport-20], Section 9
     */
    if (packet_in->pi_path_id != conn->ifc_cur_path_id
        && 0 == (conn->ifc_flags & IFC_SERVER)
        && !(packet_in->pi_path_id == conn->ifc_mig_path_id
                && migra_is_on(conn, conn->ifc_mig_path_id)))
    {
        /* The "known server address" is recorded in the current path. */
        switch ((NP_IS_IPv6(CUR_NPATH(conn)) << 1) |
                 NP_IS_IPv6(&conn->ifc_paths[packet_in->pi_path_id].cop_path))
        {
        case (1 << 1) | 1:  /* IPv6 */
            if (lsquic_sockaddr_eq(NP_PEER_SA(CUR_NPATH(conn)), NP_PEER_SA(
                        &conn->ifc_paths[packet_in->pi_path_id].cop_path)))
                goto known_peer_addr;
            break;
        case (0 << 1) | 0:  /* IPv4 */
            if (lsquic_sockaddr_eq(NP_PEER_SA(CUR_NPATH(conn)), NP_PEER_SA(
                        &conn->ifc_paths[packet_in->pi_path_id].cop_path)))
                goto known_peer_addr;
            break;
        }
        LSQ_DEBUG("ignore packet from unknown server address");
        return 0;
    }
  known_peer_addr:

    /* The packet is decrypted before receive history is updated.  This is
     * done to make sure that a bad packet won't occupy a slot in receive
     * history and subsequent good packet won't be marked as a duplicate.
     */
    if (0 == (packet_in->pi_flags & PI_DECRYPTED))
    {
        dec_packin = conn->ifc_conn.cn_esf_c->esf_decrypt_packet(
                            conn->ifc_conn.cn_enc_session, conn->ifc_enpub,
                            &conn->ifc_conn, packet_in);
        switch (dec_packin)
        {
        case DECPI_BADCRYPT:
        case DECPI_TOO_SHORT:
            if (conn->ifc_enpub->enp_settings.es_honor_prst
                /* In server mode, even if we do support stateless reset packets,
                 * they are handled in lsquic_engine.c.  No need to have this
                 * logic here.
                 */
                && !(conn->ifc_flags & IFC_SERVER)
                                        && is_stateless_reset(conn, packet_in))
            {
                LSQ_INFO("received stateless reset packet: aborting connection");
                conn->ifc_flags |= IFC_GOT_PRST;
                return -1;
            }
            else if (dec_packin == DECPI_BADCRYPT)
            {
                CONN_STATS(in.undec_packets, 1);
                LSQ_INFO("could not decrypt packet (type %s)",
                                    lsquic_hety2str[packet_in->pi_header_type]);
                return 0;
            }
            else
            {
                CONN_STATS(in.undec_packets, 1);
                LSQ_INFO("packet is too short to be decrypted");
                return 0;
            }
        case DECPI_NOT_YET:
            return 0;
        case DECPI_NOMEM:
            return 0;
        case DECPI_VIOLATION:
            ABORT_QUIETLY(0, TEC_PROTOCOL_VIOLATION,
                                    "decrypter reports protocol violation");
            return -1;
        case DECPI_OK:
            /* Receiving any other type of packet precludes subsequent retries.
             * We only set it if decryption is successful.
             */
            conn->ifc_flags |= IFC_RETRIED;
            break;
        }
    }

    is_dcid_changed = !LSQUIC_CIDS_EQ(CN_SCID(&conn->ifc_conn),
                                        &packet_in->pi_dcid);
    if (pns == PNS_INIT)
        conn->ifc_conn.cn_esf.i->esfi_set_iscid(conn->ifc_conn.cn_enc_session,
                                                                    packet_in);
    else
    {
        if (is_dcid_changed)
        {
            if (LSQUIC_CIDS_EQ(&conn->ifc_conn.cn_cces[0].cce_cid,
                            &packet_in->pi_dcid)
                && !(conn->ifc_conn.cn_cces[0].cce_flags & CCE_SEQNO))
            {
                ABORT_QUIETLY(0, TEC_PROTOCOL_VIOLATION,
                            "protocol violation detected bad dcid");
                return -1;
            }
        }
        if (pns == PNS_HSK)
        {
            if ((conn->ifc_flags & (IFC_SERVER | IFC_IGNORE_INIT)) == IFC_SERVER)
                ignore_init(conn);
            lsquic_send_ctl_maybe_calc_rough_rtt(&conn->ifc_send_ctl, pns - 1);
        }
    }
    EV_LOG_PACKET_IN(LSQUIC_LOG_CONN_ID, packet_in);

    is_rechist_empty = lsquic_rechist_is_empty(&conn->ifc_rechist[pns]);
    st = lsquic_rechist_received(&conn->ifc_rechist[pns], packet_in->pi_packno,
                                                    packet_in->pi_received);
    switch (st) {
    case REC_ST_OK:
        if (!(conn->ifc_flags & (IFC_SERVER|IFC_DCID_SET)))
            record_dcid(conn, packet_in);
        saved_path_id = conn->ifc_cur_path_id;
        parse_regular_packet(conn, packet_in);
        if (saved_path_id == conn->ifc_cur_path_id)
        {
            if (conn->ifc_cur_path_id != packet_in->pi_path_id)
            {
                if (0 != on_new_or_unconfirmed_path(conn, packet_in))
                {
                    LSQ_DEBUG("path %hhu invalid, cancel any path response "
                        "on it", packet_in->pi_path_id);
                    conn->ifc_send_flags &= ~(SF_SEND_PATH_RESP
                                                    << packet_in->pi_path_id);
                }
            }
            else if (is_dcid_changed
                && !LSQUIC_CIDS_EQ(CN_SCID(&conn->ifc_conn),
                                   &packet_in->pi_dcid))
            {
                if (0 != on_dcid_change(conn, &packet_in->pi_dcid))
                    return -1;
            }
        }
        if (lsquic_packet_in_non_probing(packet_in)
                        && packet_in->pi_packno > conn->ifc_max_non_probing)
            conn->ifc_max_non_probing = packet_in->pi_packno;
        /* From [draft-ietf-quic-transport-30] Section 13.2.1:
         *
 " In order to assist loss detection at the sender, an endpoint SHOULD
 " generate and send an ACK frame without delay when it receives an ack-
 " eliciting packet either:
 "
 " *  when the received packet has a packet number less than another
 "    ack-eliciting packet that has been received, or
 "
 " *  when the packet has a packet number larger than the highest-
 "    numbered ack-eliciting packet that has been received and there are
 "    missing packets between that packet and this packet.
        *
        */
        if (packet_in->pi_frame_types & IQUIC_FRAME_ACKABLE_MASK)
        {
            if (PNS_APP == pns /* was_missing is only used in PNS_APP */)
            {
                if (packet_in->pi_packno > conn->ifc_max_ackable_packno_in)
                {
                    was_missing = (enum was_missing)    /* WM_MAX_GAP is 1 */
                        !is_rechist_empty /* Don't count very first packno */
                        && conn->ifc_max_ackable_packno_in + 1
                                                    < packet_in->pi_packno
                        && holes_after(&conn->ifc_rechist[PNS_APP],
                            conn->ifc_max_ackable_packno_in);
                    conn->ifc_max_ackable_packno_in = packet_in->pi_packno;
                }
                else
                    was_missing = (enum was_missing)    /* WM_SMALLER is 2 */
                    /* The check is necessary (rather setting was_missing to
                     * WM_SMALLER) because we cannot guarantee that peer does
                     * not have bugs.
                     */
                        ((packet_in->pi_packno
                                    < conn->ifc_max_ackable_packno_in) << 1);
            }
            else
                was_missing = WM_NONE;
            ++conn->ifc_n_slack_akbl[pns];
        }
        else
            was_missing = WM_NONE;
        conn->ifc_n_slack_all += PNS_APP == pns;
        if (0 == (conn->ifc_flags & (IFC_ACK_QUED_INIT << pns)))
        {
            if (PNS_APP == pns)
                try_queueing_ack_app(conn, was_missing,
                    lsquic_packet_in_ecn(packet_in), packet_in->pi_received);
            else
                try_queueing_ack_init_or_hsk(conn, pns);
        }
        conn->ifc_incoming_ecn <<= 1;
        conn->ifc_incoming_ecn |=
                            lsquic_packet_in_ecn(packet_in) != ECN_NOT_ECT;
        ++conn->ifc_ecn_counts_in[pns][ lsquic_packet_in_ecn(packet_in) ];
        if (PNS_APP == pns
                && (cpath = &conn->ifc_paths[packet_in->pi_path_id],
                                            cpath->cop_flags & COP_SPIN_BIT)
                /* [draft-ietf-quic-transport-30] Section 17.3.1 talks about
                 * how spin bit value is set.
                 */
                && (packet_in->pi_packno > cpath->cop_max_packno
                    /* Zero means "unset", in which case any incoming packet
                     * number will do.  On receipt of second packet numbered
                     * zero, the rechist module will dup it and this code path
                     * won't hit.
                     */
                    || cpath->cop_max_packno == 0))
        {
            cpath->cop_max_packno = packet_in->pi_packno;
            if (conn->ifc_flags & IFC_SERVER)
                cpath->cop_spin_bit = lsquic_packet_in_spin_bit(packet_in);
            else
                cpath->cop_spin_bit = !lsquic_packet_in_spin_bit(packet_in);
        }
        conn->ifc_pub.bytes_in += packet_in->pi_data_sz;
        if ((conn->ifc_mflags & MF_VALIDATE_PATH) &&
                (packet_in->pi_header_type == HETY_SHORT
              || packet_in->pi_header_type == HETY_HANDSHAKE))
        {
            conn->ifc_mflags &= ~MF_VALIDATE_PATH;
            lsquic_send_ctl_path_validated(&conn->ifc_send_ctl);
        }
        return 0;
    case REC_ST_DUP:
        CONN_STATS(in.dup_packets, 1);
        LSQ_INFO("packet %"PRIu64" is a duplicate", packet_in->pi_packno);
        return 0;
    default:
        assert(0);
        /* Fall through */
    case REC_ST_ERR:
        CONN_STATS(in.err_packets, 1);
        LSQ_INFO("error processing packet %"PRIu64, packet_in->pi_packno);
        return -1;
    }
}