uint32_t picoquic_protect_packet(picoquic_cnx_t* cnx,
    picoquic_packet_type_enum ptype,
    uint8_t * bytes,
    picoquic_path_t* path_x,
    uint64_t sequence_number,
    uint32_t length, uint32_t header_length,
    uint8_t* send_buffer, uint32_t send_buffer_max,
    picoquic_packet_header *ph,
    void * aead_context, void* pn_enc)
{
    uint32_t send_length;
    uint32_t h_length;
    uint32_t pn_offset = 0;
    size_t sample_offset = 0;
    uint32_t pn_length = 0;
    uint32_t aead_checksum_length;
    
    if(aead_context != NULL){
        aead_checksum_length = (uint32_t)picoquic_aead_get_checksum_length(aead_context);
    }else{
        return 0;
    }

    /* Create the packet header just before encrypting the content */
    h_length = picoquic_create_packet_header(cnx, ptype, path_x,
        sequence_number, send_buffer, &pn_offset, &pn_length);
    /* Make sure that the payload length is encoded in the header */
    /* Using encryption, the "payload" length also includes the encrypted packet length */
    picoquic_update_payload_length(send_buffer, pn_offset, h_length - pn_length, length + aead_checksum_length);

    picoquic_cnx_t *pcnx = cnx;
    if (ph != NULL) {
        picoquic_parse_packet_header(cnx->quic, send_buffer, length + aead_checksum_length, (struct sockaddr *) &path_x->local_addr, ph, &pcnx, false);
    }

    /* If fuzzing is required, apply it*/
    if (cnx->quic->fuzz_fn != NULL) {
        if (h_length == header_length) {
            memcpy(bytes, send_buffer, header_length);
        }
        length = cnx->quic->fuzz_fn(cnx->quic->fuzz_ctx, cnx, bytes,
            send_buffer_max - aead_checksum_length, length, header_length);
        if (h_length == header_length) {
            memcpy(send_buffer, bytes, header_length);
        }
    }

    /* Encrypt the packet */
    send_length = (uint32_t)picoquic_aead_encrypt_generic(send_buffer + /* header_length */ h_length,
        bytes + header_length, length - header_length,
        sequence_number, send_buffer, /* header_length */ h_length, aead_context);

    send_length += /* header_length */ h_length;

    /* if needed, log the segment */
    if (cnx->quic->F_log != NULL) {
        picoquic_log_outgoing_segment(cnx->quic->F_log, 1, cnx,
                                      bytes, sequence_number, length,
                                      send_buffer, send_length);
    }

    /* Next, encrypt the PN -- The sample is located after the pn_offset */
    sample_offset = /* header_length */ pn_offset + 4;

    if (pn_offset < sample_offset)
    {
        uint8_t first_mask = (ph->ptype == picoquic_packet_1rtt_protected_phi0 || ph->ptype == picoquic_packet_1rtt_protected_phi1) ? 0x1F : 0x0F;
        /* This is always true, as use pn_length = 4 */
        uint8_t mask[5] = { 0, 0, 0, 0, 0 };
        uint8_t pn_l;

        picoquic_hp_encrypt(pn_enc, send_buffer + sample_offset, mask, mask, 5);
        /* Encode the first byte */
        pn_l = (send_buffer[0] & 3) + 1;
        send_buffer[0] ^= (mask[0] & first_mask);

        /* Packet encoding is 1 to 4 bytes */
        for (uint8_t i = 0; i < pn_l; i++) {
            send_buffer[pn_offset+i] ^= mask[i+1];
        }
    }


    return send_length;
}