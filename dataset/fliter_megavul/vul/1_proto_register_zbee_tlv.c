void proto_register_zbee_tlv(void)
{
    /* NCP protocol headers */
    static hf_register_info hf[] = {
        { &hf_zbee_tlv_relay_msg_type,
        { "Type", "zbee_tlv.relay.type", FT_UINT8, BASE_HEX, VALS(zbee_aps_relay_tlvs), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_relay_msg_length,
        { "Length", "zbee_tlv.relay.length", FT_UINT8, BASE_DEC, NULL, 0x0,  NULL, HFILL }},

        { &hf_zbee_tlv_relay_msg_joiner_ieee,
        { "Joiner IEEE",        "zbee_tlv.relay.joiner_ieee", FT_EUI64, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},

        { &hf_zbee_tlv_global_type,
          { "Type",        "zbee_tlv.type_global", FT_UINT8, BASE_HEX,
            VALS(zbee_tlv_global_types), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_local_type_key_update_req_rsp,
          { "Type",        "zbee_tlv.type_local", FT_UINT8, BASE_HEX,
            VALS(zbee_tlv_local_types_key_update_req_rsp), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_local_type_key_negotiation_req_rsp,
          { "Type",        "zbee_tlv.type_local", FT_UINT8, BASE_HEX,
            VALS(zbee_tlv_local_types_key_negotiation_req_rsp), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_local_type_get_auth_level_rsp,
          { "Type",        "zbee_tlv.type_local", FT_UINT8, BASE_HEX,
            VALS(zbee_tlv_local_types_get_auth_level_rsp), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_local_type_clear_all_bindings_req,
          { "Type",        "zbee_tlv.type_local", FT_UINT8, BASE_HEX,
            VALS(zbee_tlv_local_types_clear_all_bindings_req), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_local_type_req_security_get_auth_token,
          { "Type",        "zbee_tlv.type_local", FT_UINT8, BASE_HEX,
            VALS(zbee_tlv_local_types_req_security_get_auth_token), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_local_type_req_security_get_auth_level,
          { "Type",        "zbee_tlv.type_local", FT_UINT8, BASE_HEX,
            VALS(zbee_tlv_local_types_req_security_get_auth_level), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_local_type_req_security_decommission,
          { "Type",        "zbee_tlv.type_local", FT_UINT8, BASE_HEX,
            VALS(zbee_tlv_local_types_req_security_decommission), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_local_type_req_beacon_survey,
          { "Type",        "zbee_tlv.type_local", FT_UINT8, BASE_HEX,
            VALS(zbee_tlv_local_types_req_beacon_survey), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_local_type_rsp_beacon_survey,
          { "Type",        "zbee_tlv.type_local", FT_UINT8, BASE_HEX,
            VALS(zbee_tlv_local_types_rsp_beacon_survey), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_local_type_req_challenge,
          { "Type",        "zbee_tlv.type_local", FT_UINT8, BASE_HEX,
            VALS(zbee_tlv_local_types_req_challenge), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_local_type_rsp_challenge,
          { "Type",        "zbee_tlv.type_local", FT_UINT8, BASE_HEX,
            VALS(zbee_tlv_local_types_rsp_challenge), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_local_type_rsp_set_configuration,
          { "Type",        "zbee_tlv.type_local", FT_UINT8, BASE_HEX,
            VALS(zbee_tlv_local_types_rsp_set_configuration), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_type,
          { "Unknown Type", "zbee_tlv.type", FT_UINT8, BASE_HEX,
            NULL, 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_length,
          { "Length",      "zbee_tlv.length", FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_zbee_tlv_value,
          { "Value",       "zbee_tlv.value", FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},

        { &hf_zbee_tlv_count,
          { "Count",       "zbee_tlv.count", FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_zbee_tlv_local_status_count,
            { "TLV Status Count",           "zbee_tlv.tlv_status_count", FT_UINT8, BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_zbee_tlv_local_type_id,
            { "TLV Type ID",                "zbee_tlv.tlv_type_id", FT_UINT8, BASE_HEX, VALS(zbee_tlv_global_types), 0x0,
            NULL, HFILL }},

        { &hf_zbee_tlv_local_proc_status,
            { "TLV Processing Status",      "zbee_tlv.tlv_proc_status", FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_zbee_tlv_manufacturer_specific,
          { "ZigBee Manufacturer ID", "zbee_tlv.manufacturer_specific", FT_UINT16, BASE_HEX, NULL,
            0x0, NULL, HFILL }},

        { &hf_zbee_tlv_supported_key_negotiation_methods,
          { "Supported Key Negotiation Methods", "zbee_tlv.supported_key_negotiation_methods", FT_UINT8, BASE_HEX, NULL,
            0x0, NULL, HFILL }},

        { &hf_zbee_tlv_supported_key_negotiation_methods_key_request,
          { "Key Request (ZigBee 3.0)",             "zbee_tlv.supported_key_negotiation_methods.key_request", FT_BOOLEAN, 8, NULL,
            ZBEE_TLV_SUPPORTED_KEY_NEGOTIATION_METHODS_KEY_REQUEST, NULL, HFILL }},

        { &hf_zbee_tlv_supported_key_negotiation_methods_ecdhe_using_curve25519_aes_mmo128,
          { "ECDHE using Curve25519 with Hash AES-MMO-128", "zbee_tlv.supported_key_negotiation_methods.ecdhe_using_curve25519_aes_mmo128", FT_BOOLEAN, 8, NULL,
            ZBEE_TLV_SUPPORTED_KEY_NEGOTIATION_METHODS_ANONYMOUS_ECDHE_USING_CURVE25519_AES_MMO128, NULL, HFILL }},

        { &hf_zbee_tlv_supported_key_negotiation_methods_ecdhe_using_curve25519_sha256,
          { "ECDHE using Curve25519 with Hash SHA-256", "zbee_tlv.supported_key_negotiation_methods.ecdhe_using_curve25519_sha256", FT_BOOLEAN, 8, NULL,
            ZBEE_TLV_SUPPORTED_KEY_NEGOTIATION_METHODS_ANONYMOUS_ECDHE_USING_CURVE25519_SHA256, NULL, HFILL }},

        { &hf_zbee_tlv_supported_secrets,
          { "Supported Pre-shared Secrets Bitmask", "zbee_tlv.supported_secrets", FT_UINT8, BASE_HEX, NULL,
            0x0, NULL, HFILL }},

        { &hf_zbee_tlv_supported_preshared_secrets_auth_token,
          { "Symmetric Authentication Token", "zbee_tlv.supported_secrets.auth_token", FT_BOOLEAN, 8, NULL,
            0x1, NULL, HFILL }},

        { &hf_zbee_tlv_supported_preshared_secrets_ic,
          { "128-bit pre-configured link-key from install code", "zbee_tlv.supported_secrets.ic", FT_BOOLEAN, 8, NULL,
            0x2, NULL, HFILL }},

        { &hf_zbee_tlv_supported_preshared_secrets_passcode_pake,
          { "Variable-length pass code for PAKE protocols", "zbee_tlv.supported_secrets.passcode_pake", FT_BOOLEAN, 8, NULL,
            0x4, NULL, HFILL }},

        { &hf_zbee_tlv_supported_preshared_secrets_basic_access_key,
          { "Basic Access Key", "zbee_tlv.supported_secrets.basic_key", FT_BOOLEAN, 8, NULL,
            0x8, NULL, HFILL }},

        { &hf_zbee_tlv_supported_preshared_secrets_admin_access_key,
          { "Administrative Access Key", "zbee_tlv.supported_secrets.admin_key", FT_BOOLEAN, 8, NULL,
            0x10, NULL, HFILL }},

        { &hf_zbee_tlv_panid_conflict_cnt,
          { "PAN ID Conflict Count", "zbee_tlv.panid_conflict_cnt", FT_UINT16, BASE_DEC, NULL,
            0x0, NULL, HFILL }},

        { &hf_zbee_tlv_next_pan_id,
          { "Next PAN ID Change", "zbee_tlv.next_pan_id", FT_UINT16, BASE_HEX, NULL,
            0x0, NULL, HFILL }},

        { &hf_zbee_tlv_next_channel_change,
          { "Next Channel Change", "zbee_tlv.next_channel", FT_UINT32, BASE_HEX, NULL,
            0x0, NULL, HFILL }},

        { &hf_zbee_tlv_passphrase,
          { "128-bit Symmetric Passphrase", "zbee_tlv.passphrase", FT_BYTES, BASE_NONE, NULL,
            0x0, NULL, HFILL }},

        { &hf_zbee_tlv_challenge_value,
          { "Challenge Value", "zbee_tlv.challenge_val", FT_BYTES, BASE_NONE, NULL,
            0x0, NULL, HFILL }},

        { &hf_zbee_tlv_aps_frame_counter,
          { "APS Frame Counter", "zbee_tlv.aps_frame_cnt", FT_UINT32, BASE_HEX, NULL,
            0x0, NULL, HFILL }},

        { &hf_zbee_tlv_challenge_counter,
          { "Challenge Counter", "zbee_tlv.challenge_cnt", FT_UINT32, BASE_HEX, NULL,
            0x0, NULL, HFILL }},

        { &hf_zbee_tlv_configuration_param,
          { "Configuration Parameters", "zbee_tlv.configuration_parameters", FT_UINT16, BASE_HEX, NULL,
            0x0, NULL, HFILL }},

        { &hf_zbee_tlv_configuration_param_restricted_mode,
          { "apsZdoRestrictedMode", "zbee_tlv.conf_param.restricted_mode", FT_UINT16, BASE_DEC, NULL,
            0x1, NULL, HFILL }},

        { &hf_zbee_tlv_configuration_param_link_key_enc,
          { "requireLinkKeyEncryptionForApsTransportKey", "zbee_tlv.conf_param.req_link_key_enc", FT_UINT16, BASE_DEC, NULL,
            0x2, NULL, HFILL }},

        { &hf_zbee_tlv_configuration_param_leave_req_allowed,
          { "nwkLeaveRequestAllowed", "zbee_tlv.conf_param.leave_req_allowed", FT_UINT16, BASE_DEC, NULL,
            0x4, NULL, HFILL }},

        { &hf_zbee_tlv_dev_cap_ext_capability_information,
          { "Capability Information", "zbee_tlv.dev_cap_ext_cap_info", FT_UINT16, BASE_HEX, NULL,
            0x0, NULL, HFILL }},

        { &hf_zbee_tlv_dev_cap_ext_zbdirect_virt_device,
          { "Zigbee Direct Virtual Device", "zbee_tlv.dev_cap_ext.zbdirect_virt_dev", FT_UINT16, BASE_DEC, NULL,
            0x1, NULL, HFILL }},

        { &hf_zbee_tlv_lqa,
          { "LQA", "zbee_tlv.lqa", FT_UINT8, BASE_HEX, NULL, 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_router_information,
          { "Router Information", "zbee_tlv.router_information", FT_UINT16, BASE_HEX, NULL,
            0x0, NULL, HFILL }},

        { &hf_zbee_tlv_router_information_hub_connectivity,
          { "Hub Connectivity",   "zbee_tlv.router_information.hub_connectivity", FT_BOOLEAN, 16, NULL,
              ZBEE_TLV_ROUTER_INFORMATION_HUB_CONNECTIVITY, NULL, HFILL }},

        { &hf_zbee_tlv_router_information_uptime,
          { "Uptime",             "zbee_tlv.router_information.uptime", FT_BOOLEAN, 16, NULL,
              ZBEE_TLV_ROUTER_INFORMATION_UPTIME, NULL, HFILL }},

        { &hf_zbee_tlv_router_information_pref_parent,
          { "Preferred parent",        "zbee_tlv.router_information.pref_parent", FT_BOOLEAN, 16, NULL,
              ZBEE_TLV_ROUTER_INFORMATION_PREF_PARENT, NULL, HFILL }},

        { &hf_zbee_tlv_router_information_battery_backup,
          { "Battery Backup",     "zbee_tlv.router_information.battery", FT_BOOLEAN, 16, NULL,
              ZBEE_TLV_ROUTER_INFORMATION_BATTERY_BACKUP, NULL, HFILL }},

        { &hf_zbee_tlv_router_information_enhanced_beacon_request_support,
          { "Enhanced Beacon Request Support", "zbee_tlv.router_information.enhanced_beacon", FT_BOOLEAN, 16, NULL,
              ZBEE_TLV_ROUTER_INFORMATION_ENHANCED_BEACON_REQUEST_SUPPORT, NULL, HFILL }},

        { &hf_zbee_tlv_router_information_mac_data_poll_keepalive_support,
          { "MAC Data Poll Keepalive Support", "zbee_tlv.router_information.mac_data_poll_keepalive", FT_BOOLEAN, 16, NULL,
              ZBEE_TLV_ROUTER_INFORMATION_MAC_DATA_POLL_KEEPALIVE_SUPPORT, NULL, HFILL }},

        { &hf_zbee_tlv_router_information_end_device_keepalive_support,
          { "End Device Keepalive Support", "zbee_tlv.router_information.end_dev_keepalive", FT_BOOLEAN, 16, NULL,
              ZBEE_TLV_ROUTER_INFORMATION_END_DEVICE_KEEPALIVE_SUPPORT, NULL, HFILL }},

        { &hf_zbee_tlv_router_information_power_negotiation_support,
          { "Power Negotiation Support", "zbee_tlv.router_information.power_negotiation", FT_BOOLEAN, 16, NULL,
              ZBEE_TLV_ROUTER_INFORMATION_POWER_NEGOTIATION_SUPPORT, NULL, HFILL }},

        { &hf_zbee_tlv_node_id,
          { "Node ID", "zbee_tlv.node_id", FT_UINT16, BASE_HEX, NULL,
            0x0, NULL, HFILL }},

        { &hf_zbee_tlv_frag_opt,
          { "Fragmentation Options", "zbee_tlv.frag_opt", FT_UINT8, BASE_HEX, NULL,
            0x0, NULL, HFILL }},

        { &hf_zbee_tlv_max_reassembled_buf_size,
          { "Maximum Reassembled Input Buffer Size", "zbee_tlv.max_buf_size", FT_UINT16, BASE_HEX, NULL,
            0x0, NULL, HFILL }},

        { &hf_zbee_tlv_selected_key_negotiation_method,
          { "Selected Key Negotiation Method", "zbee_tlv.selected_key_negotiation_method", FT_UINT8, BASE_HEX,
            VALS(zbee_tlv_selected_key_negotiation_method), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_selected_pre_shared_secret,
          { "Selected Pre Shared Secret", "zbee_tlv.selected_pre_shared_secret", FT_UINT8, BASE_HEX,
            VALS(zbee_tlv_selected_pre_shared_secret), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_device_eui64,
          { "Device EUI64", "zbee_tlv.device_eui64", FT_EUI64, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},

        { &hf_zbee_tlv_public_point,
          { "Public Point", "zbee_tlv.public_point", FT_BYTES, BASE_NONE, NULL,
            0x0, NULL, HFILL }},

        { &hf_zbee_tlv_global_tlv_id,
          { "TLV Type ID", "zbee_tlv.global_tlv_id", FT_UINT8, BASE_HEX, VALS(zbee_tlv_global_types), 0x0,
            NULL, HFILL }},

        { &hf_zbee_tlv_local_ieee_addr,
          { "IEEE Addr", "zbee_tlv.ieee_addr", FT_EUI64, BASE_NONE, NULL, 0x0,
            NULL, HFILL }},

        { &hf_zbee_tlv_mic64,
          { "MIC", "zbee_tlv.mic64", FT_UINT64, BASE_HEX, NULL, 0x0,
            NULL, HFILL }},

        { &hf_zbee_tlv_local_initial_join_method,
          { "Initial Join Method",        "zbee_tlv.init_method", FT_UINT8, BASE_HEX,
            VALS(zbee_initial_join_methods), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_local_active_lk_type,
          { "Active link key type",        "zbee_tlv.lk_type", FT_UINT8, BASE_HEX,
            VALS(zbee_active_lk_types), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_zbd_comm_tlv,
            { "ZBD Commissioning Service TLV Type ID", "zbee_tlv.zbd.comm_tlv_id", FT_UINT8, BASE_HEX,
              VALS(zbee_tlv_zbd_comm_types), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_zbd_comm_mj_cmd_tlv,
            { "ZBD Manage Joiners TLV Type ID", "zbee_tlv.zbd.comm_mj_tlv_id", FT_UINT8, BASE_HEX,
              VALS(zbee_tlv_zbd_comm_mj_types), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_zbd_secur_tlv,
            { "ZBD Manage Joiners TLV Type ID", "zbee_tlv.zbd.comm_mj_tlv_id", FT_UINT8, BASE_HEX,
              VALS(zbee_tlv_zbd_secur_types), 0x0, NULL, HFILL }},

        { &hf_zbee_tlv_local_tunneling_npdu,
            { "NPDU", "zbee_tlv.zbd.npdu", FT_NONE, BASE_NONE,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_zbd_tunneling_npdu_msg_tlv,
            { "NPDU Message TLV", "zbee_tlv.zbd.tlv.tunneling.npdu_msg", FT_NONE, BASE_NONE,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_ext_pan_id,
            { "Extended PAN ID", "zbee_tlv.zbd.comm.ext_pan_id", FT_BYTES, SEP_COLON,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_short_pan_id,
            { "Short PAN ID", "zbee_tlv.zbd.comm.short_pan_id", FT_UINT16, BASE_HEX,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_channel_mask,
            { "Network Channels", "zbee_tlv.zbd.comm.nwk_channel_mask", FT_UINT32, BASE_HEX,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_channel_page,
            { "Channel Page", "zbee_tlv.zbd.comm.nwk_channel_page", FT_UINT8, BASE_DEC,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_channel_page_count,
            { "Channel Page Count", "zbee_tlv.zbd.comm.nwk_channel_page_count", FT_UINT8, BASE_DEC,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_nwk_key,
            { "Network key", "zbee_tlv.zbd.comm.nwk_key", FT_BYTES, BASE_NONE,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_link_key,
            { "Link key", "zbee_tlv.zbd.comm.link_key", FT_BYTES, BASE_NONE,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_dev_type,
            { "Device type", "zbee_tlv.zbd.comm.dev_type", FT_UINT8, BASE_HEX,
                VALS(zbee_tlv_local_types_dev_type_str), 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_nwk_addr,
            { "Network address", "zbee_tlv.zbd.comm.nwk_addr", FT_UINT16, BASE_HEX,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_join_method,
            { "Join method", "zbee_tlv.zbd.comm.join_method", FT_UINT8, BASE_HEX,
                VALS(zbee_tlv_local_types_join_method_str), 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_tc_addr,
            { "TC address", "zbee_tlv.zbd.comm.tc_addr", FT_UINT64, BASE_HEX,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_nwk_upd_id,
            { "Network update ID", "zbee_tlv.zbd.comm.nwk_upd_id", FT_UINT8, BASE_HEX,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_key_seq_num,
            { "Network active key sequence number", "zbee_tlv.zbd.comm.nwk_key_seq_num", FT_UINT8, BASE_HEX,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_adm_key,
            { "Admin key", "zbee_tlv.zbd.comm.admin_key", FT_BYTES, BASE_NONE,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_status_code_domain,
            { "Domain", "zbee_tlv.zbd.comm.status_code_domain", FT_UINT8, BASE_HEX,
                VALS(zbee_tlv_local_types_status_code_domain_str), 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_status_code_value,
            { "Code", "zbee_tlv.zbd.comm.status_code_value", FT_UINT8, BASE_HEX,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_mj_prov_lnk_key,
            { "Manage Joiners Provisional Link key", "zbee_tlv.zbd.comm.manage_joiners_prov_lnk_key", FT_BYTES, BASE_NONE,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_mj_ieee_addr,
            { "Manage Joiners IEEE Address", "zbee_tlv.zbd.comm.manage_joiners_ieee_addr", FT_UINT64, BASE_HEX,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_mj_cmd,
            { "Manage Joiners command", "zbee_tlv.zbd.comm.manage_joiners_cmd", FT_UINT8, BASE_HEX,
                VALS(zbee_tlv_local_types_mj_cmd_str), 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_tunneling_npdu_flags,
            { "NPDU Flags", "zbee_tlv.zbd.tunneling.npdu_flags", FT_UINT8, BASE_HEX,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_tunneling_npdu_flags_security,
            { "Security Enabled", "zbee_tlv.zbd.tunneling.npdu_flags.security", FT_BOOLEAN, 8,
                NULL, 0b00000001, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_tunneling_npdu_flags_reserved,
            { "Reserved", "zbee_tlv.zbd.tunneling.npdu_flags.reserved", FT_UINT8, BASE_DEC,
                NULL, 0b11111110, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_tunneling_npdu_length,
            { "NPDU Length", "zbee_tlv.zbd.tunneling.npdu_length", FT_UINT8, BASE_DEC,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_selected_key_method,
            { "Selected Key Negotiation Method", "zbee_tlv.zbd.secur.key_method", FT_UINT8, BASE_HEX,
                VALS(zbee_tlv_local_types_key_method_str), 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_selected_psk_secret,
            { "Selected PSK Secret", "zbee_tlv.zbd.secur.psk_secret", FT_UINT8, BASE_HEX,
                VALS(zbee_tlv_local_types_psk_secret_str), 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_nwk_key_seq_num,
            { "Network Key Sequence Number", "zbee_tlv.zbd.secur.nwk_key_seq_num", FT_UINT8, BASE_DEC,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_mac_tag,
            { "MAC Tag", "zbee_tlv.zbd.secur.mac_tag", FT_BYTES, BASE_NONE,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_link_key_flags,
            { "Link Key", "zbee_tlv.zbd.comm.join.link_key", FT_UINT8, BASE_HEX,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_link_key_flags_unique,
            { "Unique", "zbee_tlv.zbd.comm.join.link_key.unique", FT_UINT8, BASE_DEC,
                VALS(zbee_tlv_local_types_lnk_key_unique_str), ZBEE_TLV_LINK_KEY_UNIQUE, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_link_key_flags_provisional,
            { "Provisional", "zbee_tlv.zbd.comm.join.link_key.provisional", FT_UINT8, BASE_DEC,
                VALS(zbee_tlv_local_types_lnk_key_provisional_str), ZBEE_TLV_LINK_KEY_PROVISIONAL, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_network_status_map,
            { "Network Status Map", "zbee_tlv.zbd.comm.status_map", FT_UINT8, BASE_HEX,
                NULL, 0x0, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_network_status_map_joined_status,
            { "Joined", "zbee_tlv.zbd.comm.status_map.joined_status", FT_UINT8, BASE_HEX,
                VALS(zbee_tlv_local_types_joined_status_str), ZBEE_TLV_STATUS_MAP_JOINED_STATUS, NULL, HFILL }
        },
        { &hf_zbee_tlv_local_comm_network_status_map_open_status,
            { "Open/Closed", "zbee_tlv.zbd.comm.status_map.open_status", FT_UINT8, BASE_DEC,
                VALS(zbee_tlv_local_types_nwk_state_str), ZBEE_TLV_STATUS_MAP_OPEN_STATUS, NULL, HFILL }
        },
        { &hf_zbee_tlv_network_status_map_network_type,
            { "Network Type", "zbee_tlv.zbd.comm.status_map.network_type", FT_UINT8, BASE_DEC,
                VALS(zbee_tlv_local_types_nwk_type_str), ZBEE_TLV_STATUS_MAP_NETWORK_TYPE, NULL, HFILL }
        },
    };

    /* Protocol subtrees */
    static gint *ett[] =
        {
            &ett_zbee_aps_tlv,
            &ett_zbee_aps_relay,
            &ett_zbee_tlv,
            &ett_zbee_tlv_supported_key_negotiation_methods,
            &ett_zbee_tlv_supported_secrets,
            &ett_zbee_tlv_router_information,
            &ett_zbee_tlv_configuration_param,
            &ett_zbee_tlv_capability_information,
            &ett_zbee_tlv_zbd_tunneling_npdu,
            &ett_zbee_tlv_zbd_tunneling_npdu_flags,
            &ett_zbee_tlv_link_key_flags,
            &ett_zbee_tlv_network_status_map
        };

    proto_zbee_tlv = proto_register_protocol("Zigbee TLV", "ZB TLV", "zbee_tlv");

    proto_register_field_array(proto_zbee_tlv, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    register_dissector("zbee_tlv", dissect_zbee_tlv_default, proto_zbee_tlv);
    zbee_nwk_handle = find_dissector("zbee_nwk");
}