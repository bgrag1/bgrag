MESSAGE_HANDLE message_clone(MESSAGE_HANDLE source_message)
{
    MESSAGE_HANDLE result;

    if (source_message == NULL)
    {
        /* Codes_SRS_MESSAGE_01_062: [If `source_message` is NULL, `message_clone` shall fail and return NULL.] */
        LogError("NULL source_message");
        result = NULL;
    }
    else
    {
        /* Codes_SRS_MESSAGE_01_003: [`message_clone` shall clone a message entirely and on success return a non-NULL handle to the cloned message.] */
        result = (MESSAGE_HANDLE)message_create();
        if (result == NULL)
        {
            /* Codes_SRS_MESSAGE_01_004: [If allocating memory for the new cloned message fails, `message_clone` shall fail and return NULL.] */
            LogError("Cannot clone message");
        }
        else
        {
            result->message_format = source_message->message_format;

            if (source_message->header != NULL)
            {
                /* Codes_SRS_MESSAGE_01_005: [If a header exists on the source message it shall be cloned by using `header_clone`.] */
                result->header = header_clone(source_message->header);
                if (result->header == NULL)
                {
                    /* Codes_SRS_MESSAGE_01_012: [ If any cloning operation for the members of the source message fails, then `message_clone` shall fail and return NULL. ]*/
                    LogError("Cannot clone message header");
                    message_destroy(result);
                    result = NULL;
                }
            }

            if ((result != NULL) && (source_message->delivery_annotations != NULL))
            {
                /* Codes_SRS_MESSAGE_01_006: [If delivery annotations exist on the source message they shall be cloned by using `annotations_clone`.] */
                result->delivery_annotations = annotations_clone(source_message->delivery_annotations);
                if (result->delivery_annotations == NULL)
                {
                    /* Codes_SRS_MESSAGE_01_012: [ If any cloning operation for the members of the source message fails, then `message_clone` shall fail and return NULL. ]*/
                    LogError("Cannot clone delivery annotations");
                    message_destroy(result);
                    result = NULL;
                }
            }

            if ((result != NULL) && (source_message->message_annotations != NULL))
            {
                /* Codes_SRS_MESSAGE_01_007: [If message annotations exist on the source message they shall be cloned by using `annotations_clone`.] */
                result->message_annotations = annotations_clone(source_message->message_annotations);
                if (result->message_annotations == NULL)
                {
                    /* Codes_SRS_MESSAGE_01_012: [ If any cloning operation for the members of the source message fails, then `message_clone` shall fail and return NULL. ]*/
                    LogError("Cannot clone message annotations");
                    message_destroy(result);
                    result = NULL;
                }
            }

            if ((result != NULL) && (source_message->properties != NULL))
            {
                /* Codes_SRS_MESSAGE_01_008: [If message properties exist on the source message they shall be cloned by using `properties_clone`.] */
                result->properties = properties_clone(source_message->properties);
                if (result->properties == NULL)
                {
                    /* Codes_SRS_MESSAGE_01_012: [ If any cloning operation for the members of the source message fails, then `message_clone` shall fail and return NULL. ]*/
                    LogError("Cannot clone message properties");
                    message_destroy(result);
                    result = NULL;
                }
            }

            if ((result != NULL) && (source_message->application_properties != NULL))
            {
                /* Codes_SRS_MESSAGE_01_009: [If application properties exist on the source message they shall be cloned by using `amqpvalue_clone`.] */
                result->application_properties = amqpvalue_clone(source_message->application_properties);
                if (result->application_properties == NULL)
                {
                    /* Codes_SRS_MESSAGE_01_012: [ If any cloning operation for the members of the source message fails, then `message_clone` shall fail and return NULL. ]*/
                    LogError("Cannot clone application annotations");
                    message_destroy(result);
                    result = NULL;
                }
            }

            if ((result != NULL) && (source_message->footer != NULL))
            {
                /* Codes_SRS_MESSAGE_01_010: [If a footer exists on the source message it shall be cloned by using `annotations_clone`.] */
                result->footer = amqpvalue_clone(source_message->footer);
                if (result->footer == NULL)
                {
                    /* Codes_SRS_MESSAGE_01_012: [ If any cloning operation for the members of the source message fails, then `message_clone` shall fail and return NULL. ]*/
                    LogError("Cannot clone message footer");
                    message_destroy(result);
                    result = NULL;
                }
            }

            if ((result != NULL) && (source_message->body_amqp_data_count > 0))
            {
                size_t i;

                result->body_amqp_data_items = (BODY_AMQP_DATA*)calloc(1, (source_message->body_amqp_data_count * sizeof(BODY_AMQP_DATA)));
                if (result->body_amqp_data_items == NULL)
                {
                    /* Codes_SRS_MESSAGE_01_012: [ If any cloning operation for the members of the source message fails, then `message_clone` shall fail and return NULL. ]*/
                    LogError("Cannot allocate memory for body data sections");
                    message_destroy(result);
                    result = NULL;
                }
                else
                {
                    for (i = 0; i < source_message->body_amqp_data_count; i++)
                    {
                        result->body_amqp_data_items[i].body_data_section_length = source_message->body_amqp_data_items[i].body_data_section_length;

                        /* Codes_SRS_MESSAGE_01_011: [If an AMQP data has been set as message body on the source message it shall be cloned by allocating memory for the binary payload.] */
                        result->body_amqp_data_items[i].body_data_section_bytes = (unsigned char*)malloc(source_message->body_amqp_data_items[i].body_data_section_length);
                        if (result->body_amqp_data_items[i].body_data_section_bytes == NULL)
                        {
                            LogError("Cannot allocate memory for body data section %u", (unsigned int)i);
                            break;
                        }
                        else
                        {
                            (void)memcpy(result->body_amqp_data_items[i].body_data_section_bytes, source_message->body_amqp_data_items[i].body_data_section_bytes, result->body_amqp_data_items[i].body_data_section_length);
                        }
                    }

                    result->body_amqp_data_count = i;
                    if (i < source_message->body_amqp_data_count)
                    {
                        /* Codes_SRS_MESSAGE_01_012: [ If any cloning operation for the members of the source message fails, then `message_clone` shall fail and return NULL. ]*/
                        message_destroy(result);
                        result = NULL;
                    }
                }
            }

            if ((result != NULL) && (source_message->body_amqp_sequence_count > 0))
            {
                size_t i;

                result->body_amqp_sequence_items = (AMQP_VALUE*)calloc(1, (source_message->body_amqp_sequence_count * sizeof(AMQP_VALUE)));
                if (result->body_amqp_sequence_items == NULL)
                {
                    /* Codes_SRS_MESSAGE_01_012: [ If any cloning operation for the members of the source message fails, then `message_clone` shall fail and return NULL. ]*/
                    LogError("Cannot allocate memory for body AMQP sequences");
                    message_destroy(result);
                    result = NULL;
                }
                else
                {
                    for (i = 0; i < source_message->body_amqp_sequence_count; i++)
                    {
                        /* Codes_SRS_MESSAGE_01_160: [ If AMQP sequences are set as AMQP body they shall be cloned by calling `amqpvalue_clone`. ] */
                        result->body_amqp_sequence_items[i] = amqpvalue_clone(source_message->body_amqp_sequence_items[i]);
                        if (result->body_amqp_sequence_items[i] == NULL)
                        {
                            LogError("Cannot clone AMQP sequence %u", (unsigned int)i);
                            break;
                        }
                    }

                    result->body_amqp_sequence_count = i;
                    if (i < source_message->body_amqp_sequence_count)
                    {
                        /* Codes_SRS_MESSAGE_01_012: [ If any cloning operation for the members of the source message fails, then `message_clone` shall fail and return NULL. ]*/
                        message_destroy(result);
                        result = NULL;
                    }
                }
            }

            if ((result != NULL) && (source_message->body_amqp_value != NULL))
            {
                /* Codes_SRS_MESSAGE_01_159: [If an AMQP value has been set as message body on the source message it shall be cloned by calling `amqpvalue_clone`. ]*/
                result->body_amqp_value = amqpvalue_clone(source_message->body_amqp_value);
                if (result->body_amqp_value == NULL)
                {
                    /* Codes_SRS_MESSAGE_01_012: [ If any cloning operation for the members of the source message fails, then `message_clone` shall fail and return NULL. ]*/
                    LogError("Cannot clone body AMQP value");
                    message_destroy(result);
                    result = NULL;
                }
            }
        }
    }

    return result;
}