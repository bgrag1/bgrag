int message_add_body_amqp_data(MESSAGE_HANDLE message, BINARY_DATA amqp_data)
{
    int result;

    /* Codes_SRS_MESSAGE_01_088: [ If `message` is NULL, `message_add_body_amqp_data` shall fail and return a non-zero value. ]*/
    if ((message == NULL) ||
        /* Tests_SRS_MESSAGE_01_089: [ If the `bytes` member of `amqp_data` is NULL and the `size` member is non-zero, `message_add_body_amqp_data` shall fail and return a non-zero value. ]*/
        ((amqp_data.bytes == NULL) &&
         (amqp_data.length != 0)))
    {
        LogError("Bad arguments: message = %p, bytes = %p, length = %u",
            message, amqp_data.bytes, (unsigned int)amqp_data.length);
        result = MU_FAILURE;
    }
    else
    {
        MESSAGE_BODY_TYPE body_type = internal_get_body_type(message);
        if ((body_type == MESSAGE_BODY_TYPE_SEQUENCE) ||
            (body_type == MESSAGE_BODY_TYPE_VALUE))
        {
            /* Codes_SRS_MESSAGE_01_091: [ If the body was already set to an AMQP value or a list of AMQP sequences, `message_add_body_amqp_data` shall fail and return a non-zero value. ]*/
            LogError("Body type already set");
            result = MU_FAILURE;
        }
        else
        {
            /* Codes_SRS_MESSAGE_01_086: [ `message_add_body_amqp_data` shall add the contents of `amqp_data` to the list of AMQP data values for the body of the message identified by `message`. ]*/
            BODY_AMQP_DATA* new_body_amqp_data_items = (BODY_AMQP_DATA*)realloc(message->body_amqp_data_items, sizeof(BODY_AMQP_DATA) * (message->body_amqp_data_count + 1));
            if (new_body_amqp_data_items == NULL)
            {
                /* Codes_SRS_MESSAGE_01_153: [ If allocating memory to store the added AMQP data fails, `message_add_body_amqp_data` shall fail and return a non-zero value. ]*/
                LogError("Cannot allocate memory for body AMQP data items");
                result = MU_FAILURE;
            }
            else
            {
                message->body_amqp_data_items = new_body_amqp_data_items;

                if (amqp_data.length == 0)
                {
                    message->body_amqp_data_items[message->body_amqp_data_count].body_data_section_bytes = NULL;
                    message->body_amqp_data_items[message->body_amqp_data_count].body_data_section_length = 0;
                    message->body_amqp_data_count++;

                    /* Codes_SRS_MESSAGE_01_087: [ On success it shall return 0. ]*/
                    result = 0;
                }
                else
                {
                    message->body_amqp_data_items[message->body_amqp_data_count].body_data_section_bytes = (unsigned char*)malloc(amqp_data.length);
                    if (message->body_amqp_data_items[message->body_amqp_data_count].body_data_section_bytes == NULL)
                    {
                        /* Codes_SRS_MESSAGE_01_153: [ If allocating memory to store the added AMQP data fails, `message_add_body_amqp_data` shall fail and return a non-zero value. ]*/
                        LogError("Cannot allocate memory for body AMQP data to be added");
                        result = MU_FAILURE;
                    }
                    else
                    {
                        message->body_amqp_data_items[message->body_amqp_data_count].body_data_section_length = amqp_data.length;
                        (void)memcpy(message->body_amqp_data_items[message->body_amqp_data_count].body_data_section_bytes, amqp_data.bytes, amqp_data.length);
                        message->body_amqp_data_count++;

                        /* Codes_SRS_MESSAGE_01_087: [ On success it shall return 0. ]*/
                        result = 0;
                    }
                }
            }
        }
    }

    return result;
}