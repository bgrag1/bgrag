config_create(void)
{
	struct config_file* cfg;
	cfg = (struct config_file*)calloc(1, sizeof(struct config_file));
	if(!cfg)
		return NULL;
	/* the defaults if no config is present */
	cfg->verbosity = 1;
	cfg->stat_interval = 0;
	cfg->stat_cumulative = 0;
	cfg->stat_extended = 0;
	cfg->stat_inhibit_zero = 1;
	cfg->num_threads = 1;
	cfg->port = UNBOUND_DNS_PORT;
	cfg->do_ip4 = 1;
	cfg->do_ip6 = 1;
	cfg->do_udp = 1;
	cfg->do_tcp = 1;
	cfg->tcp_reuse_timeout = 60 * 1000; /* 60s in milisecs */
	cfg->max_reuse_tcp_queries = 200;
	cfg->tcp_upstream = 0;
	cfg->udp_upstream_without_downstream = 0;
	cfg->tcp_mss = 0;
	cfg->outgoing_tcp_mss = 0;
	cfg->tcp_idle_timeout = 30 * 1000; /* 30s in millisecs */
	cfg->tcp_auth_query_timeout = 3 * 1000; /* 3s in millisecs */
	cfg->do_tcp_keepalive = 0;
	cfg->tcp_keepalive_timeout = 120 * 1000; /* 120s in millisecs */
	cfg->sock_queue_timeout = 0; /* do not check timeout */
	cfg->ssl_service_key = NULL;
	cfg->ssl_service_pem = NULL;
	cfg->ssl_port = UNBOUND_DNS_OVER_TLS_PORT;
	cfg->ssl_upstream = 0;
	cfg->tls_cert_bundle = NULL;
	cfg->tls_win_cert = 0;
	cfg->tls_use_sni = 1;
	cfg->https_port = UNBOUND_DNS_OVER_HTTPS_PORT;
	if(!(cfg->http_endpoint = strdup("/dns-query"))) goto error_exit;
	cfg->http_max_streams = 100;
	cfg->http_query_buffer_size = 4*1024*1024;
	cfg->http_response_buffer_size = 4*1024*1024;
	cfg->http_nodelay = 1;
	cfg->use_syslog = 1;
	cfg->log_identity = NULL; /* changed later with argv[0] */
	cfg->log_time_ascii = 0;
	cfg->log_queries = 0;
	cfg->log_replies = 0;
	cfg->log_tag_queryreply = 0;
	cfg->log_local_actions = 0;
	cfg->log_servfail = 0;
	cfg->log_destaddr = 0;
#ifndef USE_WINSOCK
#  ifdef USE_MINI_EVENT
	/* select max 1024 sockets */
	cfg->outgoing_num_ports = 960;
	cfg->num_queries_per_thread = 512;
#  else
	/* libevent can use many sockets */
	cfg->outgoing_num_ports = 4096;
	cfg->num_queries_per_thread = 1024;
#  endif
	cfg->outgoing_num_tcp = 10;
	cfg->incoming_num_tcp = 10;
#else
	cfg->outgoing_num_ports = 48; /* windows is limited in num fds */
	cfg->num_queries_per_thread = 24;
	cfg->outgoing_num_tcp = 2; /* leaves 64-52=12 for: 4if,1stop,thread4 */
	cfg->incoming_num_tcp = 2;
#endif
	cfg->stream_wait_size = 4 * 1024 * 1024;
	cfg->edns_buffer_size = 1232; /* from DNS flagday recommendation */
	cfg->msg_buffer_size = 65552; /* 64 k + a small margin */
	cfg->msg_cache_size = 4 * 1024 * 1024;
	cfg->msg_cache_slabs = 4;
	cfg->jostle_time = 200;
	cfg->rrset_cache_size = 4 * 1024 * 1024;
	cfg->rrset_cache_slabs = 4;
	cfg->host_ttl = 900;
	cfg->bogus_ttl = 60;
	cfg->min_ttl = 0;
	cfg->max_ttl = 3600 * 24;
	cfg->max_negative_ttl = 3600;
	cfg->min_negative_ttl = 0;
	cfg->prefetch = 0;
	cfg->prefetch_key = 0;
	cfg->deny_any = 0;
	cfg->infra_cache_slabs = 4;
	cfg->infra_cache_numhosts = 10000;
	cfg->infra_cache_min_rtt = 50;
	cfg->infra_cache_max_rtt = 120000;
	cfg->infra_keep_probing = 0;
	cfg->delay_close = 0;
	cfg->udp_connect = 1;
	if(!(cfg->outgoing_avail_ports = (int*)calloc(65536, sizeof(int))))
		goto error_exit;
	init_outgoing_availports(cfg->outgoing_avail_ports, 65536);
	if(!(cfg->username = strdup(UB_USERNAME))) goto error_exit;
#ifdef HAVE_CHROOT
	if(!(cfg->chrootdir = strdup(CHROOT_DIR))) goto error_exit;
#endif
	if(!(cfg->directory = strdup(RUN_DIR))) goto error_exit;
	if(!(cfg->logfile = strdup(""))) goto error_exit;
	if(!(cfg->pidfile = strdup(PIDFILE))) goto error_exit;
	if(!(cfg->target_fetch_policy = strdup("3 2 1 0 0"))) goto error_exit;
	cfg->fast_server_permil = 0;
	cfg->fast_server_num = 3;
	cfg->donotqueryaddrs = NULL;
	cfg->donotquery_localhost = 1;
	cfg->root_hints = NULL;
	cfg->use_systemd = 0;
	cfg->do_daemonize = 1;
	cfg->if_automatic = 0;
	cfg->if_automatic_ports = NULL;
	cfg->so_rcvbuf = 0;
	cfg->so_sndbuf = 0;
	cfg->so_reuseport = REUSEPORT_DEFAULT;
	cfg->ip_transparent = 0;
	cfg->ip_freebind = 0;
	cfg->ip_dscp = 0;
	cfg->num_ifs = 0;
	cfg->ifs = NULL;
	cfg->num_out_ifs = 0;
	cfg->out_ifs = NULL;
	cfg->stubs = NULL;
	cfg->forwards = NULL;
	cfg->auths = NULL;
#ifdef CLIENT_SUBNET
	cfg->client_subnet = NULL;
	cfg->client_subnet_zone = NULL;
	cfg->client_subnet_opcode = LDNS_EDNS_CLIENT_SUBNET;
	cfg->client_subnet_always_forward = 0;
	cfg->max_client_subnet_ipv4 = 24;
	cfg->max_client_subnet_ipv6 = 56;
	cfg->min_client_subnet_ipv4 = 0;
	cfg->min_client_subnet_ipv6 = 0;
	cfg->max_ecs_tree_size_ipv4 = 100;
	cfg->max_ecs_tree_size_ipv6 = 100;
#endif
	cfg->views = NULL;
	cfg->acls = NULL;
	cfg->tcp_connection_limits = NULL;
	cfg->harden_short_bufsize = 1;
	cfg->harden_large_queries = 0;
	cfg->harden_glue = 1;
	cfg->harden_dnssec_stripped = 1;
	cfg->harden_below_nxdomain = 1;
	cfg->harden_referral_path = 0;
	cfg->harden_algo_downgrade = 0;
	cfg->harden_unknown_additional = 0;
	cfg->use_caps_bits_for_id = 0;
	cfg->caps_whitelist = NULL;
	cfg->private_address = NULL;
	cfg->private_domain = NULL;
	cfg->unwanted_threshold = 0;
	cfg->hide_identity = 0;
	cfg->hide_version = 0;
	cfg->hide_trustanchor = 0;
	cfg->hide_http_user_agent = 0;
	cfg->identity = NULL;
	cfg->version = NULL;
	cfg->http_user_agent = NULL;
	cfg->nsid_cfg_str = NULL;
	cfg->nsid = NULL;
	cfg->nsid_len = 0;
	cfg->auto_trust_anchor_file_list = NULL;
	cfg->trust_anchor_file_list = NULL;
	cfg->trust_anchor_list = NULL;
	cfg->trusted_keys_file_list = NULL;
	cfg->trust_anchor_signaling = 1;
	cfg->root_key_sentinel = 1;
	cfg->domain_insecure = NULL;
	cfg->val_date_override = 0;
	cfg->val_sig_skew_min = 3600; /* at least daylight savings trouble */
	cfg->val_sig_skew_max = 86400; /* at most timezone settings trouble */
	cfg->val_max_restart = 5;
	cfg->val_clean_additional = 1;
	cfg->val_log_level = 0;
	cfg->val_log_squelch = 0;
	cfg->val_permissive_mode = 0;
	cfg->aggressive_nsec = 1;
	cfg->ignore_cd = 0;
	cfg->disable_edns_do = 0;
	cfg->serve_expired = 0;
	cfg->serve_expired_ttl = 0;
	cfg->serve_expired_ttl_reset = 0;
	cfg->serve_expired_reply_ttl = 30;
	cfg->serve_expired_client_timeout = 0;
	cfg->ede_serve_expired = 0;
	cfg->serve_original_ttl = 0;
	cfg->zonemd_permissive_mode = 0;
	cfg->add_holddown = 30*24*3600;
	cfg->del_holddown = 30*24*3600;
	cfg->keep_missing = 366*24*3600; /* one year plus a little leeway */
	cfg->permit_small_holddown = 0;
	cfg->key_cache_size = 4 * 1024 * 1024;
	cfg->key_cache_slabs = 4;
	cfg->neg_cache_size = 1 * 1024 * 1024;
	cfg->local_zones = NULL;
	cfg->local_zones_nodefault = NULL;
#ifdef USE_IPSET
	cfg->local_zones_ipset = NULL;
#endif
	cfg->local_zones_disable_default = 0;
	cfg->local_data = NULL;
	cfg->local_zone_overrides = NULL;
	cfg->unblock_lan_zones = 0;
	cfg->insecure_lan_zones = 0;
	cfg->python_script = NULL;
	cfg->dynlib_file = NULL;
	cfg->remote_control_enable = 0;
	cfg->control_ifs.first = NULL;
	cfg->control_ifs.last = NULL;
	cfg->control_port = UNBOUND_CONTROL_PORT;
	cfg->control_use_cert = 1;
	cfg->minimal_responses = 1;
	cfg->rrset_roundrobin = 1;
	cfg->unknown_server_time_limit = 376;
	cfg->discard_timeout = 1900; /* msec */
	cfg->wait_limit = 1000;
	cfg->wait_limit_cookie = 10000;
	cfg->wait_limit_netblock = NULL;
	cfg->wait_limit_cookie_netblock = NULL;
	cfg->max_udp_size = 1232; /* value taken from edns_buffer_size */
	if(!(cfg->server_key_file = strdup(RUN_DIR"/unbound_server.key")))
		goto error_exit;
	if(!(cfg->server_cert_file = strdup(RUN_DIR"/unbound_server.pem")))
		goto error_exit;
	if(!(cfg->control_key_file = strdup(RUN_DIR"/unbound_control.key")))
		goto error_exit;
	if(!(cfg->control_cert_file = strdup(RUN_DIR"/unbound_control.pem")))
		goto error_exit;

#ifdef CLIENT_SUBNET
	if(!(cfg->module_conf = strdup("subnetcache validator iterator"))) goto error_exit;
#else
	if(!(cfg->module_conf = strdup("validator iterator"))) goto error_exit;
#endif
	if(!(cfg->val_nsec3_key_iterations =
		strdup("1024 150 2048 150 4096 150"))) goto error_exit;
#if defined(DNSTAP_SOCKET_PATH)
	if(!(cfg->dnstap_socket_path = strdup(DNSTAP_SOCKET_PATH)))
		goto error_exit;
#endif
	cfg->dnstap_bidirectional = 1;
	cfg->dnstap_tls = 1;
	cfg->disable_dnssec_lame_check = 0;
	cfg->ip_ratelimit_cookie = 0;
	cfg->ip_ratelimit = 0;
	cfg->ratelimit = 0;
	cfg->ip_ratelimit_slabs = 4;
	cfg->ratelimit_slabs = 4;
	cfg->ip_ratelimit_size = 4*1024*1024;
	cfg->ratelimit_size = 4*1024*1024;
	cfg->ratelimit_for_domain = NULL;
	cfg->ratelimit_below_domain = NULL;
	cfg->ip_ratelimit_factor = 10;
	cfg->ratelimit_factor = 10;
	cfg->ip_ratelimit_backoff = 0;
	cfg->ratelimit_backoff = 0;
	cfg->outbound_msg_retry = 5;
	cfg->max_sent_count = 32;
	cfg->max_query_restarts = 11;
	cfg->qname_minimisation = 1;
	cfg->qname_minimisation_strict = 0;
	cfg->shm_enable = 0;
	cfg->shm_key = 11777;
	cfg->edns_client_strings = NULL;
	cfg->edns_client_string_opcode = 65001;
	cfg->dnscrypt = 0;
	cfg->dnscrypt_port = 0;
	cfg->dnscrypt_provider = NULL;
	cfg->dnscrypt_provider_cert = NULL;
	cfg->dnscrypt_provider_cert_rotated = NULL;
	cfg->dnscrypt_secret_key = NULL;
	cfg->dnscrypt_shared_secret_cache_size = 4*1024*1024;
	cfg->dnscrypt_shared_secret_cache_slabs = 4;
	cfg->dnscrypt_nonce_cache_size = 4*1024*1024;
	cfg->dnscrypt_nonce_cache_slabs = 4;
	cfg->pad_responses = 1;
	cfg->pad_responses_block_size = 468; /* from RFC8467 */
	cfg->pad_queries = 1;
	cfg->pad_queries_block_size = 128; /* from RFC8467 */
#ifdef USE_IPSECMOD
	cfg->ipsecmod_enabled = 1;
	cfg->ipsecmod_ignore_bogus = 0;
	cfg->ipsecmod_hook = NULL;
	cfg->ipsecmod_max_ttl = 3600;
	cfg->ipsecmod_whitelist = NULL;
	cfg->ipsecmod_strict = 0;
#endif
	cfg->do_answer_cookie = 0;
	memset(cfg->cookie_secret, 0, sizeof(cfg->cookie_secret));
	cfg->cookie_secret_len = 16;
	init_cookie_secret(cfg->cookie_secret, cfg->cookie_secret_len);
#ifdef USE_CACHEDB
	if(!(cfg->cachedb_backend = strdup("testframe"))) goto error_exit;
	if(!(cfg->cachedb_secret = strdup("default"))) goto error_exit;
	cfg->cachedb_no_store = 0;
	cfg->cachedb_check_when_serve_expired = 1;
#ifdef USE_REDIS
	if(!(cfg->redis_server_host = strdup("127.0.0.1"))) goto error_exit;
	cfg->redis_server_path = NULL;
	cfg->redis_server_password = NULL;
	cfg->redis_timeout = 100;
	cfg->redis_server_port = 6379;
	cfg->redis_expire_records = 0;
	cfg->redis_logical_db = 0;
#endif  /* USE_REDIS */
#endif  /* USE_CACHEDB */
#ifdef USE_IPSET
	cfg->ipset_name_v4 = NULL;
	cfg->ipset_name_v6 = NULL;
#endif
	cfg->ede = 0;
	return cfg;
error_exit:
	config_delete(cfg);
	return NULL;
}