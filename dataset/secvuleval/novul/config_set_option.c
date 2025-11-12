int config_set_option(struct config_file* cfg, const char* opt,
	const char* val)
{
	char buf[64];
	if(!opt) return 0;
	if(opt[strlen(opt)-1] != ':' && strlen(opt)+2<sizeof(buf)) {
		snprintf(buf, sizeof(buf), "%s:", opt);
		opt = buf;
	}
	S_NUMBER_OR_ZERO("verbosity:", verbosity)
	else if(strcmp(opt, "statistics-interval:") == 0) {
		if(strcmp(val, "0") == 0 || strcmp(val, "") == 0)
			cfg->stat_interval = 0;
		else if(atoi(val) == 0)
			return 0;
		else cfg->stat_interval = atoi(val);
	} else if(strcmp(opt, "num-threads:") == 0) {
		/* not supported, library must have 1 thread in bgworker */
		return 0;
	} else if(strcmp(opt, "outgoing-port-permit:") == 0) {
		return cfg_mark_ports(val, 1,
			cfg->outgoing_avail_ports, 65536);
	} else if(strcmp(opt, "outgoing-port-avoid:") == 0) {
		return cfg_mark_ports(val, 0,
			cfg->outgoing_avail_ports, 65536);
	} else if(strcmp(opt, "local-zone:") == 0) {
		return cfg_parse_local_zone(cfg, val);
	} else if(strcmp(opt, "val-override-date:") == 0) {
		if(strcmp(val, "") == 0 || strcmp(val, "0") == 0) {
			cfg->val_date_override = 0;
		} else if(strlen(val) == 14) {
			cfg->val_date_override = cfg_convert_timeval(val);
			return cfg->val_date_override != 0;
		} else {
			if(atoi(val) == 0) return 0;
			cfg->val_date_override = (uint32_t)atoi(val);
		}
	} else if(strcmp(opt, "local-data-ptr:") == 0) {
		char* ptr = cfg_ptr_reverse((char*)opt);
		return cfg_strlist_insert(&cfg->local_data, ptr);
	} else if(strcmp(opt, "logfile:") == 0) {
		cfg->use_syslog = 0;
		free(cfg->logfile);
		return (cfg->logfile = strdup(val)) != NULL;
	}
	else if(strcmp(opt, "log-time-ascii:") == 0)
	{ IS_YES_OR_NO; cfg->log_time_ascii = (strcmp(val, "yes") == 0);
	  log_set_time_asc(cfg->log_time_ascii); }
	else S_SIZET_NONZERO("max-udp-size:", max_udp_size)
	else S_YNO("use-syslog:", use_syslog)
	else S_STR("log-identity:", log_identity)
	else S_YNO("extended-statistics:", stat_extended)
	else S_YNO("statistics-inhibit-zero:", stat_inhibit_zero)
	else S_YNO("statistics-cumulative:", stat_cumulative)
	else S_YNO("shm-enable:", shm_enable)
	else S_NUMBER_OR_ZERO("shm-key:", shm_key)
	else S_YNO("do-ip4:", do_ip4)
	else S_YNO("do-ip6:", do_ip6)
	else S_YNO("do-udp:", do_udp)
	else S_YNO("do-tcp:", do_tcp)
	else S_YNO("prefer-ip4:", prefer_ip4)
	else S_YNO("prefer-ip6:", prefer_ip6)
	else S_YNO("tcp-upstream:", tcp_upstream)
	else S_YNO("udp-upstream-without-downstream:",
		udp_upstream_without_downstream)
	else S_NUMBER_NONZERO("tcp-mss:", tcp_mss)
	else S_NUMBER_NONZERO("outgoing-tcp-mss:", outgoing_tcp_mss)
	else S_NUMBER_NONZERO("tcp-auth-query-timeout:", tcp_auth_query_timeout)
	else S_NUMBER_NONZERO("tcp-idle-timeout:", tcp_idle_timeout)
	else S_NUMBER_NONZERO("max-reuse-tcp-queries:", max_reuse_tcp_queries)
	else S_NUMBER_NONZERO("tcp-reuse-timeout:", tcp_reuse_timeout)
	else S_YNO("edns-tcp-keepalive:", do_tcp_keepalive)
	else S_NUMBER_NONZERO("edns-tcp-keepalive-timeout:", tcp_keepalive_timeout)
	else S_NUMBER_OR_ZERO("sock-queue-timeout:", sock_queue_timeout)
	else S_YNO("ssl-upstream:", ssl_upstream)
	else S_YNO("tls-upstream:", ssl_upstream)
	else S_STR("ssl-service-key:", ssl_service_key)
	else S_STR("tls-service-key:", ssl_service_key)
	else S_STR("ssl-service-pem:", ssl_service_pem)
	else S_STR("tls-service-pem:", ssl_service_pem)
	else S_NUMBER_NONZERO("ssl-port:", ssl_port)
	else S_NUMBER_NONZERO("tls-port:", ssl_port)
	else S_STR("ssl-cert-bundle:", tls_cert_bundle)
	else S_STR("tls-cert-bundle:", tls_cert_bundle)
	else S_YNO("tls-win-cert:", tls_win_cert)
	else S_YNO("tls-system-cert:", tls_win_cert)
	else S_STRLIST("additional-ssl-port:", tls_additional_port)
	else S_STRLIST("additional-tls-port:", tls_additional_port)
	else S_STRLIST("tls-additional-ports:", tls_additional_port)
	else S_STRLIST("tls-additional-port:", tls_additional_port)
	else S_STRLIST_APPEND("tls-session-ticket-keys:", tls_session_ticket_keys)
	else S_STR("tls-ciphers:", tls_ciphers)
	else S_STR("tls-ciphersuites:", tls_ciphersuites)
	else S_YNO("tls-use-sni:", tls_use_sni)
	else S_NUMBER_NONZERO("https-port:", https_port)
	else S_STR("http-endpoint:", http_endpoint)
	else S_NUMBER_NONZERO("http-max-streams:", http_max_streams)
	else S_MEMSIZE("http-query-buffer-size:", http_query_buffer_size)
	else S_MEMSIZE("http-response-buffer-size:", http_response_buffer_size)
	else S_YNO("http-nodelay:", http_nodelay)
	else S_YNO("http-notls-downstream:", http_notls_downstream)
	else S_YNO("interface-automatic:", if_automatic)
	else S_STR("interface-automatic-ports:", if_automatic_ports)
	else S_YNO("use-systemd:", use_systemd)
	else S_YNO("do-daemonize:", do_daemonize)
	else S_NUMBER_NONZERO("port:", port)
	else S_NUMBER_NONZERO("outgoing-range:", outgoing_num_ports)
	else S_SIZET_OR_ZERO("outgoing-num-tcp:", outgoing_num_tcp)
	else S_SIZET_OR_ZERO("incoming-num-tcp:", incoming_num_tcp)
	else S_MEMSIZE("stream-wait-size:", stream_wait_size)
	else S_SIZET_NONZERO("edns-buffer-size:", edns_buffer_size)
	else S_SIZET_NONZERO("msg-buffer-size:", msg_buffer_size)
	else S_MEMSIZE("msg-cache-size:", msg_cache_size)
	else S_POW2("msg-cache-slabs:", msg_cache_slabs)
	else S_SIZET_NONZERO("num-queries-per-thread:",num_queries_per_thread)
	else S_SIZET_OR_ZERO("jostle-timeout:", jostle_time)
	else S_MEMSIZE("so-rcvbuf:", so_rcvbuf)
	else S_MEMSIZE("so-sndbuf:", so_sndbuf)
	else S_YNO("so-reuseport:", so_reuseport)
	else S_YNO("ip-transparent:", ip_transparent)
	else S_YNO("ip-freebind:", ip_freebind)
	else S_NUMBER_OR_ZERO("ip-dscp:", ip_dscp)
	else S_MEMSIZE("rrset-cache-size:", rrset_cache_size)
	else S_POW2("rrset-cache-slabs:", rrset_cache_slabs)
	else S_YNO("prefetch:", prefetch)
	else S_YNO("prefetch-key:", prefetch_key)
	else S_YNO("deny-any:", deny_any)
	else if(strcmp(opt, "cache-max-ttl:") == 0)
	{ IS_NUMBER_OR_ZERO; cfg->max_ttl = atoi(val); MAX_TTL=(time_t)cfg->max_ttl;}
	else if(strcmp(opt, "cache-max-negative-ttl:") == 0)
	{ IS_NUMBER_OR_ZERO; cfg->max_negative_ttl = atoi(val); MAX_NEG_TTL=(time_t)cfg->max_negative_ttl;}
	else if(strcmp(opt, "cache-min-negative-ttl:") == 0)
	{ IS_NUMBER_OR_ZERO; cfg->min_negative_ttl = atoi(val); MIN_NEG_TTL=(time_t)cfg->min_negative_ttl;}
	else if(strcmp(opt, "cache-min-ttl:") == 0)
	{ IS_NUMBER_OR_ZERO; cfg->min_ttl = atoi(val); MIN_TTL=(time_t)cfg->min_ttl;}
	else if(strcmp(opt, "infra-cache-min-rtt:") == 0) {
	 	IS_NUMBER_OR_ZERO; cfg->infra_cache_min_rtt = atoi(val);
		RTT_MIN_TIMEOUT=cfg->infra_cache_min_rtt;
	}
	else if(strcmp(opt, "infra-cache-max-rtt:") == 0) {
		IS_NUMBER_OR_ZERO; cfg->infra_cache_max_rtt = atoi(val);
		RTT_MAX_TIMEOUT=cfg->infra_cache_max_rtt;
		USEFUL_SERVER_TOP_TIMEOUT = RTT_MAX_TIMEOUT;
		BLACKLIST_PENALTY = USEFUL_SERVER_TOP_TIMEOUT*4;
	}
	else S_YNO("infra-keep-probing:", infra_keep_probing)
	else S_NUMBER_OR_ZERO("infra-host-ttl:", host_ttl)
	else S_POW2("infra-cache-slabs:", infra_cache_slabs)
	else S_SIZET_NONZERO("infra-cache-numhosts:", infra_cache_numhosts)
	else S_NUMBER_OR_ZERO("delay-close:", delay_close)
	else S_YNO("udp-connect:", udp_connect)
	else S_STR("chroot:", chrootdir)
	else S_STR("username:", username)
	else S_STR("directory:", directory)
	else S_STR("pidfile:", pidfile)
	else S_YNO("hide-identity:", hide_identity)
	else S_YNO("hide-version:", hide_version)
	else S_YNO("hide-trustanchor:", hide_trustanchor)
	else S_YNO("hide-http-user-agent:", hide_http_user_agent)
	else S_STR("identity:", identity)
	else S_STR("version:", version)
	else S_STR("http-user-agent:", http_user_agent)
	else if(strcmp(opt, "nsid:") == 0) {
		free(cfg->nsid_cfg_str);
		if (!(cfg->nsid_cfg_str = strdup(val)))
			return 0;
		/* Empty string is just validly unsetting nsid */
		if (*val == 0) {
			free(cfg->nsid);
			cfg->nsid = NULL;
			cfg->nsid_len = 0;
			return 1;
		}
		cfg->nsid = cfg_parse_nsid(val, &cfg->nsid_len);
		return cfg->nsid != NULL;
	}
	else S_STRLIST("root-hints:", root_hints)
	else S_STR("target-fetch-policy:", target_fetch_policy)
	else S_YNO("harden-glue:", harden_glue)
	else S_YNO("harden-short-bufsize:", harden_short_bufsize)
	else S_YNO("harden-large-queries:", harden_large_queries)
	else S_YNO("harden-dnssec-stripped:", harden_dnssec_stripped)
	else S_YNO("harden-below-nxdomain:", harden_below_nxdomain)
	else S_YNO("harden-referral-path:", harden_referral_path)
	else S_YNO("harden-algo-downgrade:", harden_algo_downgrade)
	else S_YNO("harden-unknown-additional:", harden_unknown_additional)
	else S_YNO("use-caps-for-id:", use_caps_bits_for_id)
	else S_STRLIST("caps-whitelist:", caps_whitelist)
	else S_SIZET_OR_ZERO("unwanted-reply-threshold:", unwanted_threshold)
	else S_STRLIST("private-address:", private_address)
	else S_STRLIST("private-domain:", private_domain)
	else S_YNO("do-not-query-localhost:", donotquery_localhost)
	else S_STRLIST("do-not-query-address:", donotqueryaddrs)
	else S_STRLIST("auto-trust-anchor-file:", auto_trust_anchor_file_list)
	else S_STRLIST("trust-anchor-file:", trust_anchor_file_list)
	else S_STRLIST("trust-anchor:", trust_anchor_list)
	else S_STRLIST("trusted-keys-file:", trusted_keys_file_list)
	else S_YNO("trust-anchor-signaling:", trust_anchor_signaling)
	else S_YNO("root-key-sentinel:", root_key_sentinel)
	else S_STRLIST("domain-insecure:", domain_insecure)
	else S_NUMBER_OR_ZERO("val-bogus-ttl:", bogus_ttl)
	else S_YNO("val-clean-additional:", val_clean_additional)
	else S_NUMBER_OR_ZERO("val-log-level:", val_log_level)
	else S_YNO("val-log-squelch:", val_log_squelch)
	else S_YNO("log-queries:", log_queries)
	else S_YNO("log-replies:", log_replies)
	else S_YNO("log-tag-queryreply:", log_tag_queryreply)
	else S_YNO("log-local-actions:", log_local_actions)
	else S_YNO("log-servfail:", log_servfail)
	else S_YNO("log-destaddr:", log_destaddr)
	else S_YNO("val-permissive-mode:", val_permissive_mode)
	else S_YNO("aggressive-nsec:", aggressive_nsec)
	else S_YNO("ignore-cd-flag:", ignore_cd)
	else S_YNO("disable-edns-do:", disable_edns_do)
	else if(strcmp(opt, "serve-expired:") == 0)
	{ IS_YES_OR_NO; cfg->serve_expired = (strcmp(val, "yes") == 0);
	  SERVE_EXPIRED = cfg->serve_expired; }
	else if(strcmp(opt, "serve-expired-ttl:") == 0)
	{ IS_NUMBER_OR_ZERO; cfg->serve_expired_ttl = atoi(val); SERVE_EXPIRED_TTL=(time_t)cfg->serve_expired_ttl;}
	else S_YNO("serve-expired-ttl-reset:", serve_expired_ttl_reset)
	else if(strcmp(opt, "serve-expired-reply-ttl:") == 0)
	{ IS_NUMBER_OR_ZERO; cfg->serve_expired_reply_ttl = atoi(val); SERVE_EXPIRED_REPLY_TTL=(time_t)cfg->serve_expired_reply_ttl;}
	else S_NUMBER_OR_ZERO("serve-expired-client-timeout:", serve_expired_client_timeout)
	else S_YNO("ede:", ede)
	else S_YNO("ede-serve-expired:", ede_serve_expired)
	else S_YNO("serve-original-ttl:", serve_original_ttl)
	else S_STR("val-nsec3-keysize-iterations:", val_nsec3_key_iterations)
	else S_YNO("zonemd-permissive-mode:", zonemd_permissive_mode)
	else S_UNSIGNED_OR_ZERO("add-holddown:", add_holddown)
	else S_UNSIGNED_OR_ZERO("del-holddown:", del_holddown)
	else S_UNSIGNED_OR_ZERO("keep-missing:", keep_missing)
	else if(strcmp(opt, "permit-small-holddown:") == 0)
	{ IS_YES_OR_NO; cfg->permit_small_holddown = (strcmp(val, "yes") == 0);
	  autr_permit_small_holddown = cfg->permit_small_holddown; }
	else S_MEMSIZE("key-cache-size:", key_cache_size)
	else S_POW2("key-cache-slabs:", key_cache_slabs)
	else S_MEMSIZE("neg-cache-size:", neg_cache_size)
	else S_YNO("minimal-responses:", minimal_responses)
	else S_YNO("rrset-roundrobin:", rrset_roundrobin)
	else S_NUMBER_OR_ZERO("unknown-server-time-limit:", unknown_server_time_limit)
	else S_NUMBER_OR_ZERO("discard-timeout:", discard_timeout)
	else S_NUMBER_OR_ZERO("wait-limit:", wait_limit)
	else S_NUMBER_OR_ZERO("wait-limit-cookie:", wait_limit_cookie)
	else S_STRLIST("local-data:", local_data)
	else S_YNO("unblock-lan-zones:", unblock_lan_zones)
	else S_YNO("insecure-lan-zones:", insecure_lan_zones)
	else S_YNO("control-enable:", remote_control_enable)
	else S_STRLIST_APPEND("control-interface:", control_ifs)
	else S_NUMBER_NONZERO("control-port:", control_port)
	else S_STR("server-key-file:", server_key_file)
	else S_STR("server-cert-file:", server_cert_file)
	else S_STR("control-key-file:", control_key_file)
	else S_STR("control-cert-file:", control_cert_file)
	else S_STR("module-config:", module_conf)
	else S_STRLIST("python-script:", python_script)
	else S_STRLIST("dynlib-file:", dynlib_file)
	else S_YNO("disable-dnssec-lame-check:", disable_dnssec_lame_check)
#ifdef CLIENT_SUBNET
	/* Can't set max subnet prefix here, since that value is used when
	 * generating the address tree. */
	/* No client-subnet-always-forward here, module registration depends on
	 * this option. */
#endif
#ifdef USE_DNSTAP
	else S_YNO("dnstap-enable:", dnstap)
	else S_YNO("dnstap-bidirectional:", dnstap_bidirectional)
	else S_STR("dnstap-socket-path:", dnstap_socket_path)
	else S_STR("dnstap-ip:", dnstap_ip)
	else S_YNO("dnstap-tls:", dnstap_tls)
	else S_STR("dnstap-tls-server-name:", dnstap_tls_server_name)
	else S_STR("dnstap-tls-cert-bundle:", dnstap_tls_cert_bundle)
	else S_STR("dnstap-tls-client-key-file:", dnstap_tls_client_key_file)
	else S_STR("dnstap-tls-client-cert-file:",
		dnstap_tls_client_cert_file)
	else S_YNO("dnstap-send-identity:", dnstap_send_identity)
	else S_YNO("dnstap-send-version:", dnstap_send_version)
	else S_STR("dnstap-identity:", dnstap_identity)
	else S_STR("dnstap-version:", dnstap_version)
	else S_YNO("dnstap-log-resolver-query-messages:",
		dnstap_log_resolver_query_messages)
	else S_YNO("dnstap-log-resolver-response-messages:",
		dnstap_log_resolver_response_messages)
	else S_YNO("dnstap-log-client-query-messages:",
		dnstap_log_client_query_messages)
	else S_YNO("dnstap-log-client-response-messages:",
		dnstap_log_client_response_messages)
	else S_YNO("dnstap-log-forwarder-query-messages:",
		dnstap_log_forwarder_query_messages)
	else S_YNO("dnstap-log-forwarder-response-messages:",
		dnstap_log_forwarder_response_messages)
#endif
#ifdef USE_DNSCRYPT
	else S_YNO("dnscrypt-enable:", dnscrypt)
	else S_NUMBER_NONZERO("dnscrypt-port:", dnscrypt_port)
	else S_STR("dnscrypt-provider:", dnscrypt_provider)
	else S_STRLIST_UNIQ("dnscrypt-provider-cert:", dnscrypt_provider_cert)
	else S_STRLIST("dnscrypt-provider-cert-rotated:", dnscrypt_provider_cert_rotated)
	else S_STRLIST_UNIQ("dnscrypt-secret-key:", dnscrypt_secret_key)
	else S_MEMSIZE("dnscrypt-shared-secret-cache-size:",
		dnscrypt_shared_secret_cache_size)
	else S_POW2("dnscrypt-shared-secret-cache-slabs:",
		dnscrypt_shared_secret_cache_slabs)
	else S_MEMSIZE("dnscrypt-nonce-cache-size:",
		dnscrypt_nonce_cache_size)
	else S_POW2("dnscrypt-nonce-cache-slabs:",
		dnscrypt_nonce_cache_slabs)
#endif
	else if(strcmp(opt, "ip-ratelimit-cookie:") == 0) {
	    IS_NUMBER_OR_ZERO; cfg->ip_ratelimit_cookie = atoi(val);
	    infra_ip_ratelimit_cookie=cfg->ip_ratelimit_cookie;
	}
	else if(strcmp(opt, "ip-ratelimit:") == 0) {
	    IS_NUMBER_OR_ZERO; cfg->ip_ratelimit = atoi(val);
	    infra_ip_ratelimit=cfg->ip_ratelimit;
	}
	else if(strcmp(opt, "ratelimit:") == 0) {
	    IS_NUMBER_OR_ZERO; cfg->ratelimit = atoi(val);
	    infra_dp_ratelimit=cfg->ratelimit;
	}
	else S_MEMSIZE("ip-ratelimit-size:", ip_ratelimit_size)
	else S_MEMSIZE("ratelimit-size:", ratelimit_size)
	else S_POW2("ip-ratelimit-slabs:", ip_ratelimit_slabs)
	else S_POW2("ratelimit-slabs:", ratelimit_slabs)
	else S_NUMBER_OR_ZERO("ip-ratelimit-factor:", ip_ratelimit_factor)
	else S_NUMBER_OR_ZERO("ratelimit-factor:", ratelimit_factor)
	else S_YNO("ip-ratelimit-backoff:", ip_ratelimit_backoff)
	else S_YNO("ratelimit-backoff:", ratelimit_backoff)
	else S_NUMBER_NONZERO("outbound-msg-retry:", outbound_msg_retry)
	else S_NUMBER_NONZERO("max-sent-count:", max_sent_count)
	else S_NUMBER_NONZERO("max-query-restarts:", max_query_restarts)
	else S_SIZET_NONZERO("fast-server-num:", fast_server_num)
	else S_NUMBER_OR_ZERO("fast-server-permil:", fast_server_permil)
	else S_YNO("qname-minimisation:", qname_minimisation)
	else S_YNO("qname-minimisation-strict:", qname_minimisation_strict)
	else S_YNO("pad-responses:", pad_responses)
	else S_SIZET_NONZERO("pad-responses-block-size:", pad_responses_block_size)
	else S_YNO("pad-queries:", pad_queries)
	else S_SIZET_NONZERO("pad-queries-block-size:", pad_queries_block_size)
	else S_STRLIST("proxy-protocol-port:", proxy_protocol_port)
#ifdef USE_IPSECMOD
	else S_YNO("ipsecmod-enabled:", ipsecmod_enabled)
	else S_YNO("ipsecmod-ignore-bogus:", ipsecmod_ignore_bogus)
	else if(strcmp(opt, "ipsecmod-max-ttl:") == 0)
	{ IS_NUMBER_OR_ZERO; cfg->ipsecmod_max_ttl = atoi(val); }
	else S_YNO("ipsecmod-strict:", ipsecmod_strict)
#endif
#ifdef USE_CACHEDB
	else S_YNO("cachedb-no-store:", cachedb_no_store)
	else S_YNO("cachedb-check-when-serve-expired:", cachedb_check_when_serve_expired)
#endif /* USE_CACHEDB */
	else if(strcmp(opt, "define-tag:") ==0) {
		return config_add_tag(cfg, val);
	/* val_sig_skew_min, max and val_max_restart are copied into val_env
	 * during init so this does not update val_env with set_option */
	} else if(strcmp(opt, "val-sig-skew-min:") == 0)
	{ IS_NUMBER_OR_ZERO; cfg->val_sig_skew_min = (int32_t)atoi(val); }
	else if(strcmp(opt, "val-sig-skew-max:") == 0)
	{ IS_NUMBER_OR_ZERO; cfg->val_sig_skew_max = (int32_t)atoi(val); }
	else if(strcmp(opt, "val-max-restart:") == 0)
	{ IS_NUMBER_OR_ZERO; cfg->val_max_restart = (int32_t)atoi(val); }
	else if (strcmp(opt, "outgoing-interface:") == 0) {
		char* d = strdup(val);
		char** oi =
		(char**)reallocarray(NULL, (size_t)cfg->num_out_ifs+1, sizeof(char*));
		if(!d || !oi) { free(d); free(oi); return -1; }
		if(cfg->out_ifs && cfg->num_out_ifs) {
			memmove(oi, cfg->out_ifs, cfg->num_out_ifs*sizeof(char*));
			free(cfg->out_ifs);
		}
		oi[cfg->num_out_ifs++] = d;
		cfg->out_ifs = oi;
	} else {
		/* unknown or unsupported (from the set_option interface):
		 * interface, outgoing-interface, access-control,
		 * stub-zone, name, stub-addr, stub-host, stub-prime
		 * forward-first, stub-first, forward-ssl-upstream,
		 * stub-ssl-upstream, forward-zone, auth-zone
		 * name, forward-addr, forward-host,
		 * ratelimit-for-domain, ratelimit-below-domain,
		 * local-zone-tag, access-control-view, interface-*,
		 * send-client-subnet, client-subnet-always-forward,
		 * max-client-subnet-ipv4, max-client-subnet-ipv6,
		 * min-client-subnet-ipv4, min-client-subnet-ipv6,
		 * max-ecs-tree-size-ipv4, max-ecs-tree-size-ipv6, ipsecmod_hook,
		 * ipsecmod_whitelist. */
		return 0;
	}
	return 1;
}