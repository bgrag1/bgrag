int picoquic_prepare_packet_server_init(picoquic_cnx_t* cnx, picoquic_path_t ** path, picoquic_packet_t* packet,
    uint64_t current_time, uint8_t* send_buffer, size_t send_buffer_max, size_t* send_length)
{
    int ret = 0;
    int tls_ready = 0;
    int epoch = 0;
    picoquic_packet_type_enum packet_type = picoquic_packet_initial;
    picoquic_packet_context_enum pc = picoquic_packet_context_initial;
    uint32_t checksum_overhead = 8;
    int is_cleartext_mode = 1;
    size_t data_bytes = 0;
    uint32_t header_length = 0;
    uint8_t* bytes = packet->bytes;
    uint32_t length = 0;
    char * reason = NULL;  // The potential reason for retransmitting a packet
    /* This packet MUST be sent on initial path */
    *path = cnx->path[0];
    picoquic_path_t* path_x = *path;

    if (cnx->crypto_context[2].aead_encrypt != NULL &&
        cnx->tls_stream[0].send_queue == NULL) {
        epoch = 2;
        pc = picoquic_packet_context_handshake;
        packet_type = picoquic_packet_handshake;
    }

    PUSH_LOG_CTX(cnx, "\"packet_type\": \"%s\"", picoquic_log_ptype_name(packet_type));

    send_buffer_max = (send_buffer_max > path_x->send_mtu) ? path_x->send_mtu : send_buffer_max;


    /* If context is handshake, verify first that there is no need for retransmit or ack
    * on initial context */
    if (ret == 0 && pc == picoquic_packet_context_handshake && cnx->crypto_context[0].aead_encrypt != NULL) {
        length = picoquic_prepare_packet_old_context(cnx, picoquic_packet_context_initial,
            path_x, packet, send_buffer_max, current_time, &header_length);
    }

    if (length == 0) {
        struct iovec *rtx_frame = (struct iovec *) queue_peek(cnx->rtx_frames[pc]);
        size_t rtx_frame_len = rtx_frame ? rtx_frame->iov_len : 0;

        checksum_overhead = picoquic_get_checksum_length(cnx, is_cleartext_mode);

        tls_ready = picoquic_is_tls_stream_ready(cnx);

        length = picoquic_predict_packet_header_length(cnx, packet_type, path_x);
        packet->ptype = packet_type;
        packet->offset = length;
        header_length = length;
        packet->sequence_number = path_x->pkt_ctx[pc].send_sequence;
        packet->send_time = current_time;
        packet->send_path = path_x;
        packet->pc = pc;

        if ((tls_ready != 0 && path_x->cwin > path_x->bytes_in_transit)
            || path_x->pkt_ctx[pc].ack_needed || rtx_frame_len > 0) {
            if (picoquic_prepare_ack_frame(cnx, current_time, pc, &bytes[length],
                send_buffer_max - checksum_overhead - length, &data_bytes)
                == 0) {
                length += (uint32_t)data_bytes;
                data_bytes = 0;
            }

            while ((rtx_frame = queue_peek(cnx->rtx_frames[pc])) != NULL &&
                   length + rtx_frame->iov_len + checksum_overhead < send_buffer_max) {
                rtx_frame = queue_dequeue(cnx->rtx_frames[pc]);
                memcpy(bytes + length, rtx_frame->iov_base, rtx_frame->iov_len);
                length += (uint32_t)rtx_frame->iov_len;
                data_bytes = rtx_frame->iov_len;
                packet->is_pure_ack = false;
                packet->is_congestion_controlled = true;
                free(rtx_frame->iov_base);
                free(rtx_frame);
            }

            /* Encode the crypto frame */
            ret = picoquic_prepare_crypto_hs_frame(cnx, epoch, &bytes[length],
                send_buffer_max - checksum_overhead - length, &data_bytes);
            if (ret == 0) {
                if (data_bytes > 0) {
                    packet->is_pure_ack = 0;
                    packet->contains_crypto = 1;
                    packet->is_congestion_controlled = 1;
                }
                length += (uint32_t)data_bytes;
            }
            else if (ret == PICOQUIC_ERROR_FRAME_BUFFER_TOO_SMALL) {
                /* todo: reset offset to previous position? */
                ret = 0;
            }

            /* progress the state if the epoch data is all sent */

            if (ret == 0 && tls_ready != 0 && data_bytes > 0 && cnx->tls_stream[epoch].send_queue == NULL) {
                if (epoch == 2 && picoquic_tls_client_authentication_activated(cnx->quic) == 0) {
                    picoquic_set_cnx_state(cnx, picoquic_state_server_ready);
                    if (cnx->callback_fn != NULL) {
                        if (cnx->callback_fn(cnx, 0, NULL, 0, picoquic_callback_almost_ready, cnx->callback_ctx, NULL) != 0) {
                            picoquic_connection_error(cnx, PICOQUIC_TRANSPORT_INTERNAL_ERROR, 0);
                        }
                    }
                }
                else {
                    picoquic_set_cnx_state(cnx, picoquic_state_server_handshake);
                }
            }

            packet->length = length;

        }
        else  if ((length = picoquic_retransmit_needed(cnx, pc, path_x, current_time, packet, send_buffer_max, &is_cleartext_mode, &header_length, &reason)) > 0) {
            if (reason != NULL) {
                protoop_id_t pid = { .id = reason };
                pid.hash = hash_value_str(pid.id);
                protoop_prepare_and_run_noparam(cnx, &pid, NULL, packet);
            }
            /* Set the new checksum length */
            checksum_overhead = picoquic_get_checksum_length(cnx, is_cleartext_mode);
            /* Check whether it makes sens to add an ACK at the end of the retransmission */
            if (picoquic_prepare_ack_frame(cnx, current_time, pc, &bytes[length],
                send_buffer_max - checksum_overhead - length, &data_bytes)
                == 0) {
                length += (uint32_t)data_bytes;
                packet->length = length;
            }
            /* document the send time & overhead */
            packet->send_time = current_time;
            packet->checksum_overhead = checksum_overhead;
            packet->is_pure_ack = 0;
        }
        else if (path_x->pkt_ctx[pc].ack_needed) {
            /* when in a handshake mode, send acks asap. */
            length = picoquic_predict_packet_header_length(cnx, packet_type, path_x);

            if (picoquic_prepare_ack_frame(cnx, current_time, pc, &bytes[length],
                send_buffer_max - checksum_overhead - length, &data_bytes)
                == 0) {
                length += (uint32_t)data_bytes;
                packet->length = length;
            }
        } else {
            length = 0;
            packet->length = 0;
        }
    }

    packet->is_congestion_controlled = 1;
    picoquic_finalize_and_protect_packet(cnx, packet,
        ret, length, header_length, checksum_overhead,
        send_length, send_buffer, (uint32_t)send_buffer_max, path_x, current_time);

    picoquic_cnx_set_next_wake_time(cnx, current_time);

    POP_LOG_CTX(cnx);

    return ret;
}