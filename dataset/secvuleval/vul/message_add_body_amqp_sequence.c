int message_add_body_amqp_sequence(MESSAGE_HANDLE message, AMQP_VALUE sequence_list)
{
    int result;

    if ((message == NULL) ||
        (sequence_list == NULL))
    {
        /* Codes_SRS_MESSAGE_01_112: [ If `message` or `sequence` is NULL, `message_add_body_amqp_sequence` shall fail and return a non-zero value. ]*/
        LogError("Bad arguments: message = %p, sequence_list = %p",
            message, sequence_list);
        result = MU_FAILURE;
    }
    else
    {
        MESSAGE_BODY_TYPE body_type = internal_get_body_type(message);
        if ((body_type == MESSAGE_BODY_TYPE_DATA) ||
            (body_type == MESSAGE_BODY_TYPE_VALUE))
        {
            /* Codes_SRS_MESSAGE_01_115: [ If the body was already set to an AMQP data list or an AMQP value, `message_add_body_amqp_sequence` shall fail and return a non-zero value. ]*/
            LogError("Body is already set to another body type");
            result = MU_FAILURE;
        }
        else
        {
            AMQP_VALUE* new_body_amqp_sequence_items = (AMQP_VALUE*)realloc(message->body_amqp_sequence_items, sizeof(AMQP_VALUE) * (message->body_amqp_sequence_count + 1));
            if (new_body_amqp_sequence_items == NULL)
            {
                /* Codes_SRS_MESSAGE_01_158: [ If allocating memory in order to store the sequence fails, `message_add_body_amqp_sequence` shall fail and return a non-zero value. ]*/
                LogError("Cannot allocate enough memory for sequence items");
                result = MU_FAILURE;
            }
            else
            {
                message->body_amqp_sequence_items = new_body_amqp_sequence_items;

                /* Codes_SRS_MESSAGE_01_110: [ `message_add_body_amqp_sequence` shall add the contents of `sequence` to the list of AMQP sequences for the body of the message identified by `message`. ]*/
                /* Codes_SRS_MESSAGE_01_156: [ The AMQP sequence shall be cloned by calling `amqpvalue_clone`. ]*/
                message->body_amqp_sequence_items[message->body_amqp_sequence_count] = amqpvalue_clone(sequence_list);
                if (message->body_amqp_sequence_items[message->body_amqp_sequence_count] == NULL)
                {
                    /* Codes_SRS_MESSAGE_01_157: [ If `amqpvalue_clone` fails, `message_add_body_amqp_sequence` shall fail and return a non-zero value. ]*/
                    LogError("Cloning sequence failed");
                    result = MU_FAILURE;
                }
                else
                {
                    /* Codes_SRS_MESSAGE_01_114: [ If adding the AMQP sequence fails, the previous value shall be preserved. ]*/
                    message->body_amqp_sequence_count++;

                    /* Codes_SRS_MESSAGE_01_111: [ On success it shall return 0. ]*/
                    result = 0;
                }
            }
        }
    }

    return result;
}