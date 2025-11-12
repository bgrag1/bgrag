picoquic_cnx_t* picoquic_create_cnx(picoquic_quic_t* quic,
    picoquic_connection_id_t initial_cnx_id, picoquic_connection_id_t remote_cnx_id,
    struct sockaddr* addr, uint64_t start_time, uint32_t preferred_version,
    char const* sni, char const* alpn, char client_mode)
{
    picoquic_cnx_t* cnx = (picoquic_cnx_t*)malloc(sizeof(picoquic_cnx_t));

    if (cnx != NULL) {
        int ret;

        memset(cnx, 0, sizeof(picoquic_cnx_t));

        cnx->quic = quic;
        cnx->client_mode = client_mode;
        /* Should return 0, since this is the first path */
        ret = picoquic_create_path(cnx, start_time, addr);

        if (ret != 0) {
            free(cnx);
            cnx = NULL;
        } else {
            cnx->next_wake_time = start_time;
            cnx->start_time = start_time;

            picoquic_insert_cnx_in_list(quic, cnx);
            picoquic_insert_cnx_by_wake_time(quic, cnx);
            /* Do not require verification for default path */
            cnx->path[0]->challenge_verified = 1;
        }
    }

    if (cnx != NULL) {
        if (quic->default_tp == NULL) {
            picoquic_init_transport_parameters(&cnx->local_parameters, cnx->client_mode);
        } else {
            memcpy(&cnx->local_parameters, quic->default_tp, sizeof(picoquic_tp_t));
        }
        if (cnx->quic->mtu_max > 0)
        {
            cnx->local_parameters.max_packet_size = cnx->quic->mtu_max;
        }


        /* Initialize local flow control variables to advertised values */
        cnx->maxdata_local = ((uint64_t)cnx->local_parameters.initial_max_data);
        cnx->max_stream_id_bidir_local = picoquic_transport_param_to_stream_id(cnx->local_parameters.initial_max_streams_bidi, cnx->client_mode, PICOQUIC_STREAM_ID_BIDIR);
        cnx->max_stream_id_bidir_local_computed = cnx->max_stream_id_bidir_local;
        cnx->max_stream_id_unidir_local = picoquic_transport_param_to_stream_id(cnx->local_parameters.initial_max_streams_uni, cnx->client_mode, PICOQUIC_STREAM_ID_UNIDIR);
        cnx->max_stream_id_unidir_local_computed = cnx->max_stream_id_unidir_local;

        /* Initialize remote variables to some plausible value.
		 * Hopefully, this will be overwritten by the parameters received in
		 * the TLS transport parameter extension */
        cnx->maxdata_remote = PICOQUIC_DEFAULT_0RTT_WINDOW;
        cnx->remote_parameters.initial_max_stream_data_bidi_remote = PICOQUIC_DEFAULT_0RTT_WINDOW;
        cnx->remote_parameters.initial_max_stream_data_uni = PICOQUIC_DEFAULT_0RTT_WINDOW;
        cnx->max_stream_id_bidir_remote = (cnx->client_mode)?4:0;
        cnx->max_stream_id_unidir_remote = 0;

        if (sni != NULL) {
            cnx->sni = picoquic_string_duplicate(sni);
        }

        if (alpn != NULL) {
            cnx->alpn = picoquic_string_duplicate(alpn);
        }

        cnx->callback_fn = quic->default_callback_fn;
        cnx->callback_ctx = quic->default_callback_ctx;
        cnx->congestion_alg = quic->default_congestion_alg;

        if (cnx->client_mode) {
            if (preferred_version == 0) {
                cnx->proposed_version = picoquic_supported_versions[0].version;
                cnx->version_index = 0;
            } else {
                cnx->version_index = picoquic_get_version_index(preferred_version);
                if (cnx->version_index < 0) {
                    cnx->version_index = PICOQUIC_INTEROP_VERSION_INDEX;
                    if ((preferred_version & 0x0A0A0A0A) == 0x0A0A0A0A) {
                        /* This is a hack, to allow greasing the cnx ID */
                        cnx->proposed_version = preferred_version;

                    } else {
                        cnx->proposed_version = picoquic_supported_versions[PICOQUIC_INTEROP_VERSION_INDEX].version;
                    }
                } else {
                    cnx->proposed_version = preferred_version;
                }
            }

            cnx->cnx_state = picoquic_state_client_init;
            if (picoquic_is_connection_id_null(initial_cnx_id)) {
                picoquic_create_random_cnx_id(quic, &initial_cnx_id, 8);
            }

            if (quic->cnx_id_callback_fn) {
                quic->cnx_id_callback_fn(cnx->path[0]->local_cnxid, picoquic_null_connection_id, quic->cnx_id_callback_ctx, &cnx->path[0]->local_cnxid);
            }

            cnx->initial_cnxid = initial_cnx_id;
        } else {
            for (int epoch = 0; epoch < PICOQUIC_NUMBER_OF_EPOCHS; epoch++) {
                cnx->tls_stream[epoch].send_queue = NULL;
            }
            cnx->cnx_state = picoquic_state_server_init;
            cnx->initial_cnxid = initial_cnx_id;
            cnx->path[0]->remote_cnxid = remote_cnx_id;

            if (quic->cnx_id_callback_fn)
                quic->cnx_id_callback_fn(cnx->path[0]->local_cnxid, cnx->initial_cnxid,
                    quic->cnx_id_callback_ctx, &cnx->path[0]->local_cnxid);

            (void)picoquic_create_cnxid_reset_secret(quic, &cnx->path[0]->local_cnxid,
                cnx->path[0]->reset_secret);

            cnx->version_index = picoquic_get_version_index(preferred_version);
            if (cnx->version_index < 0) {
                /* TODO: this is an internal error condition, should not happen */
                cnx->version_index = 0;
                cnx->proposed_version = picoquic_supported_versions[0].version;
            } else {
                cnx->proposed_version = preferred_version;
            }
        }

        if (cnx != NULL) {
            /* Moved packet context initialization into path creation */

            cnx->latest_progress_time = start_time;
            cnx->local_parameters.initial_source_connection_id = cnx->path[0]->local_cnxid;

            for (int epoch = 0; epoch < PICOQUIC_NUMBER_OF_EPOCHS; epoch++) {
                cnx->tls_stream[epoch].stream_id = 0;
                cnx->tls_stream[epoch].consumed_offset = 0;
                cnx->tls_stream[epoch].fin_offset = 0;
                cnx->tls_stream[epoch].next_stream = NULL;
                cnx->tls_stream[epoch].stream_data = NULL;
                cnx->tls_stream[epoch].sent_offset = 0;
                cnx->tls_stream[epoch].local_error = 0;
                cnx->tls_stream[epoch].remote_error = 0;
                cnx->tls_stream[epoch].maxdata_local = (uint64_t)((int64_t)-1);
                cnx->tls_stream[epoch].maxdata_remote = (uint64_t)((int64_t)-1);
                /* No need to reset the state flags, as they are not used for the crypto stream */
            }

            cnx->congestion_alg = cnx->quic->default_congestion_alg;
            if (cnx->congestion_alg != NULL) {
                cnx->congestion_alg->alg_init(cnx, cnx->path[0]);
            }
        }
    }

    if (cnx != NULL) {
        register_protocol_operations(cnx);
        /* It's already the time to inject plugins, as creating the TLS context sets up the transport parameters */
        if (quic->local_plugins.size > 0) {
            int err = plugin_insert_plugins(cnx, quic->local_plugins.size, quic->local_plugins.elems);
            if (err) {
                cnx = NULL;
            }
        }
    }

    /* Only initialize TLS after all parameters have been set */

    if (cnx != NULL) {
        if (picoquic_tlscontext_create(quic, cnx, start_time) != 0) {
            /* Cannot just do partial creation! */
            picoquic_delete_cnx(cnx);
            cnx = NULL;
        }
    }

    if (cnx != NULL) {
        if (picoquic_setup_initial_traffic_keys(cnx)) {
            /* Cannot initialize aead for initial packets */
            picoquic_delete_cnx(cnx);
            cnx = NULL;
        }
    }

    if (cnx != NULL) {
        if (!picoquic_is_connection_id_null(cnx->path[0]->local_cnxid)) {
            (void)picoquic_register_cnx_id(quic, cnx, &cnx->path[0]->local_cnxid);
        }

        if (addr != NULL) {
            (void)picoquic_register_net_id(quic, cnx, addr);
        }
    }

    if (cnx != NULL) {
        /* Also initialize reserve queue */
        cnx->reserved_frames = queue_init();
        /* And the retry queue */
        cnx->retry_frames = queue_init();
        for (int pc = 0; pc < picoquic_nb_packet_context; pc++) {
            cnx->rtx_frames[pc] = queue_init();
        }
        /* TODO change this arbitrary value */
        cnx->core_rate = 500;
    }

    return cnx;
}