protoop_arg_t schedule_frames_on_path(picoquic_cnx_t *cnx)
{
    picoquic_packet_t* packet = (picoquic_packet_t*) cnx->protoop_inputv[0];
    size_t send_buffer_max = (size_t) cnx->protoop_inputv[1];
    uint64_t current_time = (uint64_t) cnx->protoop_inputv[2];
    /* picoquic_packet_t* retransmit_p = (picoquic_packet_t*) cnx->protoop_inputv[3]; */ // Unused
    /* picoquic_path_t* from_path = (picoquic_path_t*) cnx->protoop_inputv[4]; */ // Unused
    char* reason = (char*) cnx->protoop_inputv[5];

    int ret = 0;
    uint32_t length = 0;
    int is_cleartext_mode = 0;
    uint32_t checksum_overhead = picoquic_get_checksum_length(cnx, is_cleartext_mode);

    /* FIXME cope with different path MTUs */
    picoquic_path_t *path_x = cnx->path[0];
    PUSH_LOG_CTX(cnx, "\"path\": \"%p\"", path_x);

    uint32_t send_buffer_min_max = (send_buffer_max > path_x->send_mtu) ? path_x->send_mtu : (uint32_t)send_buffer_max;
    int retransmit_possible = 1;
    picoquic_packet_context_enum pc = picoquic_packet_context_application;
    size_t data_bytes = 0;
    uint32_t header_length = 0;
    uint8_t* bytes = packet->bytes;
    picoquic_packet_type_enum packet_type = picoquic_packet_1rtt_protected_phi0;

    /* TODO: manage multiple streams. */
    picoquic_stream_head* stream = NULL;
    int tls_ready = picoquic_is_tls_stream_ready(cnx);
    stream = picoquic_find_ready_stream(cnx);
    picoquic_stream_head* plugin_stream = NULL;
    plugin_stream = picoquic_find_ready_plugin_stream(cnx);


    /* First enqueue frames that can be fairly sent, if any */
    /* Only schedule new frames if there is no planned frames */
    if (queue_peek(cnx->reserved_frames) == NULL) {
        stream = picoquic_schedule_next_stream(cnx, send_buffer_min_max - checksum_overhead - length, path_x);
        picoquic_frame_fair_reserve(cnx, path_x, stream, send_buffer_min_max - checksum_overhead - length);
    }

    char * retrans_reason = NULL;
    if (ret == 0 && retransmit_possible &&
        (length = picoquic_retransmit_needed(cnx, pc, path_x, current_time, packet, send_buffer_min_max, &is_cleartext_mode, &header_length, &retrans_reason)) > 0) {
        if (reason != NULL) {
            protoop_id_t pid = { .id = retrans_reason };
            pid.hash = hash_value_str(pid.id);
            protoop_prepare_and_run_noparam(cnx, &pid, NULL, packet);
        }
        /* Set the new checksum length */
        checksum_overhead = picoquic_get_checksum_length(cnx, is_cleartext_mode);
        /* Check whether it makes sense to add an ACK at the end of the retransmission */
        /* Don't do that if it risks mixing clear text and encrypted ack */
        if (is_cleartext_mode == 0 && packet->ptype != picoquic_packet_0rtt_protected) {
            if (picoquic_prepare_ack_frame(cnx, current_time, pc, &bytes[length],
                send_buffer_min_max - checksum_overhead - length, &data_bytes)
                == 0) {
                length += (uint32_t)data_bytes;
                packet->length = length;
            }
        }
        /* document the send time & overhead */
        packet->is_pure_ack = 0;
        packet->send_time = current_time;
        packet->checksum_overhead = checksum_overhead;
    }
    else if (ret == 0) {
        length = picoquic_predict_packet_header_length(
                cnx, packet_type, path_x);
        packet->ptype = packet_type;
        packet->offset = length;
        header_length = length;
        packet->sequence_number = path_x->pkt_ctx[pc].send_sequence;
        packet->send_time = current_time;
        packet->send_path = path_x;

        if (((stream == NULL && tls_ready == 0) ||
                path_x->cwin <= path_x->bytes_in_transit)
            && picoquic_is_ack_needed(cnx, current_time, pc, path_x) == 0
            && picoquic_should_send_max_data(cnx) == 0
            && path_x->challenge_response_to_send == 0
            && (cnx->client_mode || !cnx->handshake_done || cnx->handshake_done_sent)
            && (path_x->challenge_verified == 1 || current_time < path_x->challenge_time + path_x->retransmit_timer)
            && queue_peek(cnx->reserved_frames) == NULL
            && queue_peek(cnx->retry_frames) == NULL
            && queue_peek(cnx->rtx_frames[pc]) == NULL) {
            if (ret == 0 && send_buffer_max > path_x->send_mtu && picoquic_is_mtu_probe_needed(cnx, path_x)) {
                length = picoquic_prepare_mtu_probe(cnx, path_x, header_length, checksum_overhead, bytes);
                packet->is_mtu_probe = 1;
                packet->length = length;
                packet->is_congestion_controlled = 0;  // See DPLPMTUD
                path_x->mtu_probe_sent = 1;
                packet->is_pure_ack = 0;
            } else {
                length = 0;
                packet->offset = 0;
            }
        } else {
            if (path_x->challenge_verified == 0 &&
                current_time >= (path_x->challenge_time + path_x->retransmit_timer)) {
                if (picoquic_prepare_path_challenge_frame(cnx, &bytes[length],
                                                            send_buffer_min_max - checksum_overhead - length,
                                                            &data_bytes, path_x) == 0) {
                    length += (uint32_t) data_bytes;
                    path_x->challenge_time = current_time;
                    path_x->challenge_repeat_count++;
                    packet->is_congestion_controlled = 1;


                    if (path_x->challenge_repeat_count > PICOQUIC_CHALLENGE_REPEAT_MAX) {
                        DBG_PRINTF("%s\n", "Too many challenge retransmits, disconnect");
                        picoquic_set_cnx_state(cnx, picoquic_state_disconnected);
                        if (cnx->callback_fn) {
                            (void)(cnx->callback_fn)(cnx, 0, NULL, 0, picoquic_callback_close, cnx->callback_ctx, NULL);
                        }
                        length = 0;
                        packet->offset = 0;
                    }
                }
            }

            if (cnx->cnx_state != picoquic_state_disconnected) {
                size_t consumed = 0;
                unsigned int is_pure_ack = packet->is_pure_ack;
                ret = picoquic_scheduler_write_new_frames(cnx, &bytes[length],
                                                          send_buffer_min_max - checksum_overhead - length,
                                                          length - packet->offset, packet,
                                                          &consumed, &is_pure_ack);
                packet->is_pure_ack = is_pure_ack;
                if (!ret && consumed > send_buffer_min_max - checksum_overhead - length) {
                    ret = PICOQUIC_ERROR_FRAME_BUFFER_TOO_SMALL;
                } else if (!ret) {
                    length += consumed;
                    /* FIXME: Sorry, I'm lazy, this could be easily fixed by making this a PO.
                        * This is needed by the way the cwin is now handled. */
                    if (path_x == cnx->path[0] && (header_length != length || picoquic_is_ack_needed(cnx, current_time, pc, path_x))) {
                        if (picoquic_prepare_ack_frame(cnx, current_time, pc, &bytes[length], send_buffer_min_max - checksum_overhead - length, &data_bytes) == 0) {
                            length += (uint32_t)data_bytes;
                        }
                    }

                    if (path_x->cwin > path_x->bytes_in_transit) {
                        /* if present, send tls data */
                        if (tls_ready) {
                            ret = picoquic_prepare_crypto_hs_frame(cnx, 3, &bytes[length],
                                                                    send_buffer_min_max - checksum_overhead - length, &data_bytes);

                            if (ret == 0) {
                                length += (uint32_t)data_bytes;
                                if (data_bytes > 0)
                                {
                                    packet->is_pure_ack = 0;
                                    packet->contains_crypto = 1;
                                    packet->is_congestion_controlled = 1;
                                }
                            }
                        }

                        if (!cnx->client_mode && cnx->handshake_done && !cnx->handshake_done_sent) {
                            ret = picoquic_prepare_handshake_done_frame(cnx, bytes + length, send_buffer_min_max - checksum_overhead - length, &data_bytes);
                            if (ret == 0 && data_bytes > 0) {
                                length += (uint32_t) data_bytes;
                                packet->has_handshake_done = 1;
                                packet->is_pure_ack = 0;
                                picoquic_crypto_context_free(&cnx->crypto_context[2]);
                            }
                        }

                        /* if present, send path response. This ensures we send it on the right path */
                        if (path_x->challenge_response_to_send && send_buffer_min_max - checksum_overhead - length >= PICOQUIC_CHALLENGE_LENGTH + 1) {
                            /* This is not really clean, but it will work */
                            bytes[length] = picoquic_frame_type_path_response;
                            memcpy(&bytes[length+1], path_x->challenge_response, PICOQUIC_CHALLENGE_LENGTH);
                            path_x->challenge_response_to_send = 0;
                            length += PICOQUIC_CHALLENGE_LENGTH + 1;
                            packet->is_congestion_controlled = 1;
                        }
                        /* If necessary, encode the max data frame */
                        if (ret == 0 && 2 * cnx->data_received > cnx->maxdata_local) {
                            ret = picoquic_prepare_max_data_frame(cnx, 2 * cnx->data_received, &bytes[length],
                                                                    send_buffer_min_max - checksum_overhead - length, &data_bytes);

                            if (ret == 0) {
                                length += (uint32_t)data_bytes;
                                if (data_bytes > 0)
                                {
                                    packet->is_pure_ack = 0;
                                    packet->is_congestion_controlled = 1;
                                }
                            }
                            else if (ret == PICOQUIC_ERROR_FRAME_BUFFER_TOO_SMALL) {
                                ret = 0;
                            }
                        }
                        /* If necessary, encode the max stream data frames */
                        if (ret == 0) {
                            ret = picoquic_prepare_required_max_stream_data_frames(cnx, &bytes[length],
                                                                                    send_buffer_min_max - checksum_overhead - length, &data_bytes);
                            if (ret == 0) {
                                length += (uint32_t)data_bytes;
                                if (data_bytes > 0)
                                {
                                    packet->is_pure_ack = 0;
                                    packet->is_congestion_controlled = 1;
                                }
                            }
                        }
                        /* If required, request for plugins */
                        if (ret == 0 && !cnx->plugin_requested) {
                            int is_retransmittable = 1;
                            for (int i = 0; ret == 0 && i < cnx->pids_to_request.size; i++) {
                                ret = picoquic_write_plugin_validate_frame(cnx, &bytes[length], &bytes[send_buffer_min_max - checksum_overhead],
                                    cnx->pids_to_request.elems[i].pid_id, cnx->pids_to_request.elems[i].plugin_name, &data_bytes, &is_retransmittable);
                                if (ret == 0) {
                                    length += (uint32_t)data_bytes;
                                    if (data_bytes > 0)
                                    {
                                        packet->is_pure_ack = 0;
                                        packet->is_congestion_controlled = 1;
                                        cnx->pids_to_request.elems[i].requested = 1;
                                    }
                                }
                                else if (ret == PICOQUIC_ERROR_FRAME_BUFFER_TOO_SMALL) {
                                    ret = 0;
                                }
                            }
                            cnx->plugin_requested = 1;
                        }

                        /* Encode the plugin frame, or frames */
                        while (plugin_stream != NULL) {
                            size_t stream_bytes_max = picoquic_stream_bytes_max(cnx, send_buffer_min_max - checksum_overhead - length, header_length, bytes);
                            ret = picoquic_prepare_plugin_frame(cnx, plugin_stream, &bytes[length],
                                                                stream_bytes_max, &data_bytes);
                            if (ret == 0) {
                                length += (uint32_t)data_bytes;
                                if (data_bytes > 0)
                                {
                                    packet->is_pure_ack = 0;
                                    packet->is_congestion_controlled = 1;
                                }

                                if (stream_bytes_max > checksum_overhead + length + 8) {
                                    plugin_stream = picoquic_find_ready_plugin_stream(cnx);
                                }
                                else {
                                    break;
                                }
                            }
                            else if (ret == PICOQUIC_ERROR_FRAME_BUFFER_TOO_SMALL) {
                                ret = 0;
                                break;
                            }
                        }

                        size_t stream_bytes_max = picoquic_stream_bytes_max(cnx, send_buffer_min_max - checksum_overhead - length, header_length, bytes);
                        stream = picoquic_schedule_next_stream(cnx, stream_bytes_max, path_x);

                        /* Encode the stream frame, or frames */
                        while (stream != NULL) {
                            ret = picoquic_prepare_stream_frame(cnx, stream, &bytes[length],
                                                                stream_bytes_max, &data_bytes);
                            if (ret == 0) {
                                length += (uint32_t)data_bytes;
                                if (data_bytes > 0)
                                {
                                    packet->is_pure_ack = 0;
                                    packet->is_congestion_controlled = 1;
                                }

                                if (stream_bytes_max > checksum_overhead + length + 8) {
                                    stream_bytes_max = picoquic_stream_bytes_max(cnx, send_buffer_min_max - checksum_overhead - length, header_length, bytes);
                                    stream = picoquic_schedule_next_stream(cnx, stream_bytes_max, path_x);
                                } else {
                                    break;
                                }
                            } else if (ret == PICOQUIC_ERROR_FRAME_BUFFER_TOO_SMALL) {
                                ret = 0;
                                break;
                            }
                        }

                         if (length <= header_length) {
                             /* Mark the bandwidth estimation as application limited */
                             path_x->delivered_limited_index = path_x->delivered;
                         }
                    }
                    if (length == 0 || length == header_length) {
                        /* Don't flood the network with packets! */
                        length = 0;
                        packet->offset = 0;
                    } else if (length > 0 && length != header_length && length + checksum_overhead <= PICOQUIC_RESET_PACKET_MIN_SIZE) {
                        uint32_t pad_size = PICOQUIC_RESET_PACKET_MIN_SIZE - checksum_overhead - length + 1;
                        for (uint32_t i = 0; i < pad_size; i++) {
                            bytes[length++] = 0;
                        }
                    }
                }

                if (path_x->cwin <= path_x->bytes_in_transit + length) {
                    picoquic_congestion_algorithm_notify_func(cnx, path_x, picoquic_congestion_notification_cwin_blocked, 0, 0, 0, current_time);
                }
            }

        }
    }

    POP_LOG_CTX(cnx);
    protoop_save_outputs(cnx, path_x, length, header_length);
    return (protoop_arg_t) ret;
}