static void link_frame_received(void* context, AMQP_VALUE performative, uint32_t payload_size, const unsigned char* payload_bytes)
{
    LINK_INSTANCE* link_instance = (LINK_INSTANCE*)context;
    AMQP_VALUE descriptor = amqpvalue_get_inplace_descriptor(performative);

    if (is_attach_type_by_descriptor(descriptor))
    {
        ATTACH_HANDLE attach_handle;

        if (amqpvalue_get_attach(performative, &attach_handle) != 0)
        {
            LogError("Cannot get attach performative");
        }
        else
        {
            if ((link_instance->role == role_receiver) &&
                (attach_get_initial_delivery_count(attach_handle, &link_instance->delivery_count) != 0))
            {
                LogError("Cannot get initial delivery count");
                remove_all_pending_deliveries(link_instance, true);
                set_link_state(link_instance, LINK_STATE_DETACHED);
            }
            else
            {
                if (attach_get_max_message_size(attach_handle, &link_instance->peer_max_message_size) != 0)
                {
                    LogError("Could not retrieve peer_max_message_size from attach frame");
                }

                if ((link_instance->link_state == LINK_STATE_DETACHED) ||
                    (link_instance->link_state == LINK_STATE_HALF_ATTACHED_ATTACH_SENT))
                {
                    if (link_instance->role == role_receiver)
                    {
                        link_instance->current_link_credit = link_instance->max_link_credit;
                        send_flow(link_instance);
                    }
                    else
                    {
                        link_instance->current_link_credit = 0;
                    }

                    if (link_instance->link_state == LINK_STATE_DETACHED)
                    {
                        set_link_state(link_instance, LINK_STATE_HALF_ATTACHED_ATTACH_RECEIVED);
                    }
                    else
                    {
                        set_link_state(link_instance, LINK_STATE_ATTACHED);
                    }
                }
            }

            attach_destroy(attach_handle);
        }
    }
    else if (is_flow_type_by_descriptor(descriptor))
    {
        FLOW_HANDLE flow_handle;
        if (amqpvalue_get_flow(performative, &flow_handle) != 0)
        {
            LogError("Cannot get flow performative");
        }
        else
        {
            if (link_instance->role == role_sender)
            {
                delivery_number rcv_delivery_count;
                uint32_t rcv_link_credit;

                if (flow_get_link_credit(flow_handle, &rcv_link_credit) != 0)
                {
                    LogError("Cannot get link credit");
                    remove_all_pending_deliveries(link_instance, true);
                    set_link_state(link_instance, LINK_STATE_DETACHED);
                }
                else if (flow_get_delivery_count(flow_handle, &rcv_delivery_count) != 0)
                {
                    LogError("Cannot get delivery count");
                    remove_all_pending_deliveries(link_instance, true);
                    set_link_state(link_instance, LINK_STATE_DETACHED);
                }
                else
                {
                    link_instance->current_link_credit = rcv_delivery_count + rcv_link_credit - link_instance->delivery_count;
                    if (link_instance->current_link_credit > 0)
                    {
                        link_instance->on_link_flow_on(link_instance->callback_context);
                    }
                }
            }
        }

        flow_destroy(flow_handle);
    }
    else if (is_transfer_type_by_descriptor(descriptor))
    {
        if (link_instance->on_transfer_received != NULL)
        {
            TRANSFER_HANDLE transfer_handle;
            if (amqpvalue_get_transfer(performative, &transfer_handle) != 0)
            {
                LogError("Cannot get transfer performative");
            }
            else
            {
                AMQP_VALUE delivery_state;
                bool more;
                bool is_error;

                if (link_instance->current_link_credit == 0)
                {
                    link_instance->current_link_credit = link_instance->max_link_credit;
                    send_flow(link_instance);
                }

                more = false;
                /* Attempt to get more flag, default to false */
                (void)transfer_get_more(transfer_handle, &more);
                is_error = false;

                if (transfer_get_delivery_id(transfer_handle, &link_instance->received_delivery_id) != 0)
                {
                    /* is this not a continuation transfer? */
                    if (link_instance->received_payload_size == 0)
                    {
                        LogError("Could not get the delivery Id from the transfer performative");
                        is_error = true;
                    }
                }

                if (!is_error)
                {
                    /* If this is a continuation transfer or if this is the first chunk of a multi frame transfer */
                    if ((link_instance->received_payload_size > 0) || more)
                    {
                        unsigned char* new_received_payload;;
                        size_t realloc_size = safe_add_size_t((size_t)link_instance->received_payload_size, payload_size);
                        if (realloc_size == SIZE_MAX ||
                            (new_received_payload = (unsigned char*)realloc(link_instance->received_payload, realloc_size)) == NULL)
                        {
                            LogError("Could not allocate memory for the received payload, size:%zu", realloc_size);
                        }
                        else
                        {
                            link_instance->received_payload = new_received_payload;
                            (void)memcpy(link_instance->received_payload + link_instance->received_payload_size, payload_bytes, payload_size);
                            link_instance->received_payload_size += payload_size;
                        }
                    }

                    if (!more)
                    {
                        const unsigned char* indicate_payload_bytes;
                        uint32_t indicate_payload_size;

                        link_instance->current_link_credit--;
                        link_instance->delivery_count++;
                        /* if no previously stored chunks then simply report the current payload */
                        if (link_instance->received_payload_size > 0)
                        {
                            indicate_payload_size = link_instance->received_payload_size;
                            indicate_payload_bytes = link_instance->received_payload;
                        }
                        else
                        {
                            indicate_payload_size = payload_size;
                            indicate_payload_bytes = payload_bytes;
                        }

                        delivery_state = link_instance->on_transfer_received(link_instance->callback_context, transfer_handle, indicate_payload_size, indicate_payload_bytes);

                        if (link_instance->received_payload_size > 0)
                        {
                            free(link_instance->received_payload);
                            link_instance->received_payload = NULL;
                            link_instance->received_payload_size = 0;
                        }

                        if (delivery_state != NULL)
                        {
                            if (send_disposition(link_instance, link_instance->received_delivery_id, delivery_state) != 0)
                            {
                                LogError("Cannot send disposition frame");
                            }

                            amqpvalue_destroy(delivery_state);
                        }
                    }
                }

                transfer_destroy(transfer_handle);
            }
        }
    }
    else if (is_disposition_type_by_descriptor(descriptor))
    {
        DISPOSITION_HANDLE disposition;
        if (amqpvalue_get_disposition(performative, &disposition) != 0)
        {
            LogError("Cannot get disposition performative");
        }
        else
        {
            delivery_number first;
            delivery_number last;

            if (disposition_get_first(disposition, &first) != 0)
            {
                LogError("Cannot get first field");
            }
            else
            {
                bool settled;

                if (disposition_get_last(disposition, &last) != 0)
                {
                    last = first;
                }

                if (disposition_get_settled(disposition, &settled) != 0)
                {
                    settled = false;
                }

                if (settled && 
                    link_instance->pending_deliveries != NULL)
                {
                    LIST_ITEM_HANDLE pending_delivery = singlylinkedlist_get_head_item(link_instance->pending_deliveries);
                    while (pending_delivery != NULL)
                    {
                        LIST_ITEM_HANDLE next_pending_delivery = singlylinkedlist_get_next_item(pending_delivery);
                        ASYNC_OPERATION_HANDLE pending_delivery_operation = (ASYNC_OPERATION_HANDLE)singlylinkedlist_item_get_value(pending_delivery);
                        if (pending_delivery_operation == NULL)
                        {
                            LogError("Cannot obtain pending delivery");
                            break;
                        }
                        else
                        {
                            DELIVERY_INSTANCE* delivery_instance = (DELIVERY_INSTANCE*)GET_ASYNC_OPERATION_CONTEXT(DELIVERY_INSTANCE, pending_delivery_operation);

                            if ((delivery_instance->delivery_id >= first) && (delivery_instance->delivery_id <= last))
                            {
                                AMQP_VALUE delivery_state;
                                if (disposition_get_state(disposition, &delivery_state) == 0)
                                {
                                    delivery_instance->on_delivery_settled(delivery_instance->callback_context, delivery_instance->delivery_id, LINK_DELIVERY_SETTLE_REASON_DISPOSITION_RECEIVED, delivery_state);
                                    async_operation_destroy(pending_delivery_operation);
                                }
                                else
                                {
                                    LogError("Failed getting the disposition state");
                                }

                                if (singlylinkedlist_remove(link_instance->pending_deliveries, pending_delivery) != 0)
                                {
                                    LogError("Cannot remove pending delivery");
                                    break;
                                }
                            }

                            pending_delivery = next_pending_delivery;
                        }
                    }
                }
            }

            disposition_destroy(disposition);
        }
    }
    else if (is_detach_type_by_descriptor(descriptor))
    {
        DETACH_HANDLE detach;

        /* Set link state appropriately based on whether we received detach condition */
        if (amqpvalue_get_detach(performative, &detach) != 0)
        {
            LogError("Cannot get detach performative");
        }
        else
        {
            bool closed = false;
            ERROR_HANDLE error;

            (void)detach_get_closed(detach, &closed);

            /* Received a detach while attached */
            if (link_instance->link_state == LINK_STATE_ATTACHED)
            {
                /* Respond with ack */
                if (send_detach(link_instance, closed, NULL) != 0)
                {
                    LogError("Failed sending detach frame");
                }
            }
            /* Received a closing detach after we sent a non-closing detach. */
            else if (closed &&
                ((link_instance->link_state == LINK_STATE_HALF_ATTACHED_ATTACH_SENT) || (link_instance->link_state == LINK_STATE_HALF_ATTACHED_ATTACH_RECEIVED)) &&
                !link_instance->is_closed)
            {

                /* In this case, we MUST signal that we closed by reattaching and then sending a closing detach.*/
                if (send_attach(link_instance, link_instance->name, 0, link_instance->role) != 0)
                {
                    LogError("Failed sending attach frame");
                }

                if (send_detach(link_instance, true, NULL) != 0)
                {
                    LogError("Failed sending detach frame");
                }
            }

            if (detach_get_error(detach, &error) != 0)
            {
                error = NULL;
            }
            remove_all_pending_deliveries(link_instance, true);
            // signal link detach received in order to handle cases like redirect
            if (link_instance->on_link_detach_received_event_subscription.on_link_detach_received != NULL)
            {
                link_instance->on_link_detach_received_event_subscription.on_link_detach_received(link_instance->on_link_detach_received_event_subscription.context, error);
            }

            if (error != NULL)
            {
                set_link_state(link_instance, LINK_STATE_ERROR);
                error_destroy(error);
            }
            else
            {
                set_link_state(link_instance, LINK_STATE_DETACHED);
            }

            detach_destroy(detach);
        }
    }
}