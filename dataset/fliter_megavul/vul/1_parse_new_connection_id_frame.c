protoop_arg_t parse_new_connection_id_frame(picoquic_cnx_t* cnx)
{
    uint8_t* bytes = (uint8_t *) cnx->protoop_inputv[0];
    const uint8_t* bytes_max = (const uint8_t *) cnx->protoop_inputv[1];

    int ack_needed = 1;
    int is_retransmittable = 1;
    new_connection_id_frame_t *frame = malloc(sizeof(new_connection_id_frame_t));
    if (!frame) {
        printf("Failed to allocate memory for new_connection_id_frame_t\n");
        protoop_save_outputs(cnx, frame, ack_needed, is_retransmittable);
        return (protoop_arg_t) NULL;
    }

    if ((bytes = picoquic_frames_varint_decode(bytes + picoquic_varint_skip(bytes), bytes_max, &frame->sequence)) == NULL ||
        (bytes = picoquic_frames_varint_decode(bytes, bytes_max, &frame->retire_prior_to)) == NULL ||
        (bytes = picoquic_frames_uint8_decode(bytes, bytes_max, &frame->connection_id.id_len)) == NULL ||
        (frame->connection_id.id_len > PICOQUIC_CONNECTION_ID_MAX_SIZE) ||
        (bytes = (bytes + frame->connection_id.id_len + 16 <= bytes_max ? bytes : NULL)) == NULL)
    {
        bytes = NULL;
        picoquic_connection_error(cnx, PICOQUIC_TRANSPORT_FRAME_FORMAT_ERROR,
            picoquic_frame_type_new_connection_id);
        free(frame);
        frame = NULL;
    }
    else
    {
        /* Memory bounds have been checked, so everything should be safe now */
        memcpy(&frame->connection_id.id, bytes, frame->connection_id.id_len);
        bytes += frame->connection_id.id_len;
        memcpy(&frame->stateless_reset_token, bytes, 16);
        bytes += 16;
    }

    protoop_save_outputs(cnx, frame, ack_needed, is_retransmittable);
    return (protoop_arg_t) bytes;
}