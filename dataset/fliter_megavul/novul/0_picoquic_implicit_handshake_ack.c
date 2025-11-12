void picoquic_implicit_handshake_ack(picoquic_cnx_t* cnx, picoquic_path_t *path, picoquic_packet_context_enum pc, uint64_t current_time)
{
    picoquic_packet_t* p = path->pkt_ctx[pc].retransmit_oldest;

    /* Remove packets from the retransmit queue */
    while (p != NULL) {
        picoquic_packet_t* p_next = p->previous_packet;
        picoquic_path_t * old_path = p->send_path;

        /* Update the congestion control state for the path */
        if (old_path != NULL) {
            if (old_path->smoothed_rtt == PICOQUIC_INITIAL_RTT && old_path->rtt_variant == 0) {
                // TODO Update the path rtt somehow
            }

            if (p->is_congestion_controlled && cnx->congestion_alg != NULL) {
                picoquic_congestion_algorithm_notify_func(cnx, old_path,
                                                picoquic_congestion_notification_acknowledgement,
                                                0, p->length, 0, current_time);
            }
        }
        /* Update the number of bytes in transit and remove old packet from queue */
        /* The packet will not be placed in the "retransmitted" queue */
        (void)picoquic_dequeue_retransmit_packet(cnx, p, 1);

        p = p_next;
    }
}