int picoquic_parse_header_and_decrypt(
    picoquic_quic_t* quic,
    uint8_t* bytes,
    uint32_t length,
    uint32_t packet_length,
    struct sockaddr* addr_from,
    uint64_t current_time,
    picoquic_packet_header* ph,
    picoquic_cnx_t** pcnx,
    uint32_t * consumed,
    int * new_context_created)
{
    /* Parse the clear text header. Ret == 0 means an incorrect packet that could not be parsed */
    int already_received = 0;
    size_t decoded_length = 0;
    int ret = picoquic_parse_packet_header(quic, bytes, length, addr_from, ph, pcnx, 1);

    if (ret == 0) {
        /* TODO: clarify length, payload length, packet length -- special case of initial packet */
        length = ph->offset + ph->payload_length;
        *consumed = length;


        if (ph->ptype == picoquic_packet_initial) {
            if ((*pcnx == NULL || !(*pcnx)->client_mode)) {
                /* Create a connection context if the CI is acceptable */
                if (packet_length < PICOQUIC_ENFORCED_INITIAL_MTU) {
                    /* Unexpected packet. Reject, drop and log. */
                    ret = PICOQUIC_ERROR_INITIAL_TOO_SHORT;
                }
            }
            if (ret == 0 && *pcnx == NULL) {
                /* if listening is OK, listen */
                *pcnx = picoquic_create_cnx(quic, ph->dest_cnx_id, ph->srce_cnx_id, addr_from, current_time, ph->vn,
                                            NULL, NULL, 0);
                *new_context_created = (*pcnx == NULL) ? 0 : 1;
            }
        }

        /* TODO: replace switch by reference to epoch */

        if (*pcnx != NULL) {
            picoquic_path_t* path_from = picoquic_get_incoming_path(*pcnx, ph);
            switch (ph->ptype) {
            case picoquic_packet_version_negotiation:
                /* Packet is not encrypted */
                break;
            case picoquic_packet_initial:
                decoded_length = picoquic_decrypt_packet(*pcnx, bytes, packet_length, ph,
                    (*pcnx)->crypto_context[0].hp_dec,
                    (*pcnx)->crypto_context[0].aead_decrypt, &already_received, path_from);
                length = ph->offset + ph->payload_length;
                *consumed = length;
                break;
            case picoquic_packet_retry:
                /* packet is not encrypted, no sequence number. */
                ph->pn = 0;
                ph->pn64 = 0;
                ph->pnmask = 0;
                decoded_length = ph->payload_length;
                break;
            case picoquic_packet_handshake:
                decoded_length = picoquic_decrypt_packet(*pcnx, bytes, length, ph,
                    (*pcnx)->crypto_context[2].hp_dec,
                    (*pcnx)->crypto_context[2].aead_decrypt, &already_received, path_from);
                break;
            case picoquic_packet_0rtt_protected:
                decoded_length = picoquic_decrypt_packet(*pcnx, bytes, length, ph,
                    (*pcnx)->crypto_context[1].hp_dec,
                    (*pcnx)->crypto_context[1].aead_decrypt, &already_received, path_from);
                break;
            case picoquic_packet_1rtt_protected_phi0:
            case picoquic_packet_1rtt_protected_phi1:
                /* TODO : roll key based on PHI */
                /* AEAD Decrypt, in place */
                decoded_length = picoquic_decrypt_packet(*pcnx, bytes, length, ph,
                    (*pcnx)->crypto_context[3].hp_dec,
                    (*pcnx)->crypto_context[3].aead_decrypt, &already_received, path_from);
                break;
            default:
                /* Packet type error. Log and ignore */
                ret = PICOQUIC_ERROR_DETECTED;
                break;
            }

            /* TODO: consider the error "too soon" */
            if (decoded_length > (length - ph->offset)) {
                ret = PICOQUIC_ERROR_AEAD_CHECK;
                if (*new_context_created) {
                    picoquic_delete_cnx(*pcnx);
                    *pcnx = NULL;
                    *new_context_created = 0;
                }
            }
            else if (already_received != 0) {
                ret = PICOQUIC_ERROR_DUPLICATE;
            }
            else {
                ph->payload_length = (uint16_t)decoded_length;
            }
        }
        else if (ph->ptype == picoquic_packet_1rtt_protected_phi0 ||
            ph->ptype == picoquic_packet_1rtt_protected_phi1)
        {
            /* This may be a stateles reset */
            *pcnx = picoquic_cnx_by_net(quic, addr_from);

            if (*pcnx != NULL && length >= PICOQUIC_RESET_PACKET_MIN_SIZE &&
                memcmp(bytes + length - PICOQUIC_RESET_SECRET_SIZE,
                (*pcnx)->path[0]->reset_secret, PICOQUIC_RESET_SECRET_SIZE) == 0) {
                ret = PICOQUIC_ERROR_STATELESS_RESET;
            }
            else {
                *pcnx = NULL;
            }
        }
    }

    return ret;
}