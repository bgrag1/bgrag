int picoquic_incoming_segment(
    picoquic_quic_t* quic,
    uint8_t* bytes,
    uint32_t length,
    uint32_t packet_length,
    uint32_t * consumed,
    struct sockaddr* addr_from,
    struct sockaddr* addr_to,
    int if_index_to,
    uint64_t current_time,
    picoquic_connection_id_t * previous_dest_id,
    int *new_context_created)
{
    int ret = 0;
    picoquic_cnx_t* cnx = NULL;
    picoquic_packet_header ph;
    *new_context_created = 0;

    /* Parse the header and decrypt the packet */
    ret = picoquic_parse_header_and_decrypt(quic, bytes, length, packet_length, addr_from,
        current_time, &ph, &cnx, consumed, new_context_created);

    if (cnx != NULL) LOG {
        PUSH_LOG_CTX(cnx, "\"packet_type\": \"%s\", \"pn\": %" PRIu64, picoquic_log_ptype_name(ph.ptype), ph.pn64);
    }

    /* Verify that the segment coalescing is for the same destination ID */
    if (ret == 0) {
        if (picoquic_is_connection_id_null(*previous_dest_id)) {
            *previous_dest_id = ph.dest_cnx_id;
        }
        /* This is commented out because with multipath, we can have coalescing of several destination IDs without having issues... */
        /*
        else if (picoquic_compare_connection_id(previous_dest_id, &ph.dest_cnx_id) != 0) {
            ret = PICOQUIC_ERROR_CNXID_SEGMENT;

        }
        */
    }

    /* Log the incoming packet */
    picoquic_log_decrypted_segment(quic->F_log, 1, cnx, 1, &ph, bytes, (uint32_t)*consumed, ret);

    if (ret == 0) {
        if (cnx == NULL) {
            if (ph.version_index < 0 && ph.vn != 0) {
                /* use the result of parsing to consider version negotiation */
                picoquic_prepare_version_negotiation(quic, addr_from, addr_to, if_index_to, &ph);
            }
            else {
                /* Unexpected packet. Reject, drop and log. */
                if (!picoquic_is_connection_id_null(ph.dest_cnx_id)) {
                    picoquic_process_unexpected_cnxid(quic, length, addr_from, addr_to, if_index_to, &ph);
                }
                ret = PICOQUIC_ERROR_DETECTED;
            }
        }
        else {
            /* TO DO: Find the incoming path */
            /* TO DO: update each of the incoming functions, since the packet is already decrypted. */
            /* Hook for performing action when connection received new packet */
            picoquic_received_packet(cnx, quic->rcv_socket, quic->rcv_tos);
            picoquic_path_t *path = (picoquic_path_t *) protoop_prepare_and_run_noparam(cnx, &PROTOOP_NOPARAM_GET_INCOMING_PATH, NULL, &ph);
            picoquic_header_parsed(cnx, &ph, path, *consumed);
            if (cnx != NULL) LOG {
                PUSH_LOG_CTX(cnx, "\"path\": \"%p\"", path);
            }

            switch (ph.ptype) {
            case picoquic_packet_version_negotiation:
                if (cnx->cnx_state == picoquic_state_client_init_sent) {
                    /* Proceed with version negotiation*/
                    ret = picoquic_incoming_version_negotiation(
                        cnx, bytes, length, addr_from, &ph, current_time);
                }
                else {
                    /* This is an unexpected packet. Log and drop.*/
                    DBG_PRINTF("Unexpected packet (%d), type: %d, epoch: %d, pc: %d, pn: %d\n",
                        cnx->client_mode, ph.ptype, ph.epoch, ph.pc, (int) ph.pn);
                    ret = PICOQUIC_ERROR_DETECTED;
                }
                break;
            case picoquic_packet_initial:
                /* Initial packet: either crypto handshakes or acks. */
                if (picoquic_compare_connection_id(&ph.dest_cnx_id, &cnx->initial_cnxid) == 0 ||
                    picoquic_compare_connection_id(&ph.dest_cnx_id, &cnx->path[0]->local_cnxid) == 0) {
                    /* Verify that the source CID matches expectation */
                    if (picoquic_is_connection_id_null(cnx->path[0]->remote_cnxid)) {
                        cnx->path[0]->remote_cnxid = ph.srce_cnx_id;
                        cnx->path[0]->local_addr_len = (addr_to->sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
                        memcpy(&cnx->path[0]->local_addr, addr_to, cnx->path[0]->local_addr_len);
                    } else if (picoquic_compare_connection_id(&cnx->path[0]->remote_cnxid, &ph.srce_cnx_id) != 0) {
                        DBG_PRINTF("Error wrong srce cnxid (%d), type: %d, epoch: %d, pc: %d, pn: %d\n",
                            cnx->client_mode, ph.ptype, ph.epoch, ph.pc, (int)ph.pn);
                        ret = PICOQUIC_ERROR_UNEXPECTED_PACKET;
                    }
                    if (ret == 0) {
                        if (cnx->client_mode == 0) {
                            /* TODO: finish processing initial connection packet */
                            cnx->local_parameters.original_destination_connection_id = ph.dest_cnx_id;
                            ret = picoquic_incoming_initial(&cnx, bytes,
                                addr_from, addr_to, if_index_to, &ph, current_time, *new_context_created);
                        }
                        else {
                            /* TODO: this really depends on the current receive epoch */
                            ret = picoquic_incoming_server_cleartext(cnx, bytes, addr_to, if_index_to, &ph, current_time);
                        }
                    }
                } else {
                    DBG_PRINTF("Error detected (%d), type: %d, epoch: %d, pc: %d, pn: %d\n",
                        cnx->client_mode, ph.ptype, ph.epoch, ph.pc, (int)ph.pn);
                    ret = PICOQUIC_ERROR_DETECTED;
                }
                break;
            case picoquic_packet_retry:
                /* TODO: server retry is completely revised in the new version. */
                ret = picoquic_incoming_retry(cnx, bytes, &ph, current_time);
                break;
            case picoquic_packet_handshake:
                if (cnx->client_mode)
                {
                    ret = picoquic_incoming_server_cleartext(cnx, bytes, addr_to, if_index_to, &ph, current_time);
                }
                else
                {
                    ret = picoquic_incoming_client_cleartext(cnx, bytes, &ph, current_time);
                }
                break;
            case picoquic_packet_0rtt_protected:
                /* TODO : decrypt with 0RTT key */
                ret = picoquic_incoming_0rtt(cnx, bytes, &ph, current_time);
                break;
            case picoquic_packet_1rtt_protected_phi0:
            case picoquic_packet_1rtt_protected_phi1:
                ret = picoquic_incoming_encrypted(cnx, bytes, &ph, addr_from, current_time);
                /* TODO : roll key based on PHI */
                break;
            default:
                /* Packet type error. Log and ignore */
                DBG_PRINTF("Unexpected packet type (%d), type: %d, epoch: %d, pc: %d, pn: %d\n",
                    cnx->client_mode, ph.ptype, ph.epoch, ph.pc, (int) ph.pn);
                ret = PICOQUIC_ERROR_DETECTED;
                break;
            }
            if (cnx != NULL) LOG {
                POP_LOG_CTX(cnx);
            }
        }
    } else if (ret == PICOQUIC_ERROR_STATELESS_RESET) {
        ret = picoquic_incoming_stateless_reset(cnx);
    }

    if (ret == 0 || ret == PICOQUIC_ERROR_SPURIOUS_REPEAT) {
        if (cnx != NULL && cnx->cnx_state != picoquic_state_disconnected &&
            ph.ptype != picoquic_packet_version_negotiation) {
            /* Mark the sequence number as received */
            /* FIXME */
            picoquic_path_t* path_x = picoquic_get_incoming_path(cnx, &ph);
            ret = picoquic_record_pn_received(cnx, path_x, ph.pc, ph.pn64, current_time);
        }
        if (cnx != NULL) {
            picoquic_cnx_set_next_wake_time(cnx, current_time);
        }
    } else if (ret == PICOQUIC_ERROR_DUPLICATE) {
        /* Bad packets are dropped silently, but duplicates should be acknowledged */
        if (cnx != NULL) {
            /* FIXME */
            picoquic_path_t* path_x = picoquic_get_incoming_path(cnx, &ph);
            path_x->pkt_ctx[ph.pc].ack_needed = 1;
        }
        ret = -1;
    } else if (ret == PICOQUIC_ERROR_AEAD_CHECK || ret == PICOQUIC_ERROR_INITIAL_TOO_SHORT ||
        ret == PICOQUIC_ERROR_UNEXPECTED_PACKET || ret == PICOQUIC_ERROR_FNV1A_CHECK ||
        ret == PICOQUIC_ERROR_CNXID_CHECK ||
        ret == PICOQUIC_ERROR_RETRY || ret == PICOQUIC_ERROR_DETECTED ||
        ret == PICOQUIC_ERROR_CONNECTION_DELETED ||
        ret == PICOQUIC_ERROR_CNXID_SEGMENT) {
        /* Bad packets are dropped silently */

        DBG_PRINTF("Packet (%d) dropped, t: %d, e: %d, pc: %d, pn: %d, l: %d, ret : %x\n",
            (cnx == NULL) ? -1 : cnx->client_mode, ph.ptype, ph.epoch, ph.pc, (int)ph.pn,
            length, ret);
        ret = -1;
    } else if (ret == 1) {
        /* wonder what happened ! */
        DBG_PRINTF("Packet (%d) get ret=1, t: %d, e: %d, pc: %d, pn: %d, l: %d\n",
            (cnx == NULL) ? -1 : cnx->client_mode, ph.ptype, ph.epoch, ph.pc, (int)ph.pn, length);
        ret = -1;
    } else if (ret != 0) {
        DBG_PRINTF("Packet (%d) error, t: %d, e: %d, pc: %d, pn: %d, l: %d, ret : %x\n",
            (cnx == NULL) ? -1 : cnx->client_mode, ph.ptype, ph.epoch, ph.pc, (int)ph.pn, length, ret);
        ret = -1;
    }

    if (cnx != NULL) LOG {
        POP_LOG_CTX(cnx);
    }

    if (cnx != NULL) {
        if (!cnx->processed_transport_parameter && cnx->remote_parameters_received) {
            picoquic_handle_plugin_negotiation(cnx);
            cnx->processed_transport_parameter = 1;
        }
    }

    return ret;
}