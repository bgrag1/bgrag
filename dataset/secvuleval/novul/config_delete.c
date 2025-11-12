config_delete(struct config_file* cfg)
{
	if(!cfg) return;
	free(cfg->username);
	free(cfg->chrootdir);
	free(cfg->directory);
	free(cfg->logfile);
	free(cfg->pidfile);
	free(cfg->if_automatic_ports);
	free(cfg->target_fetch_policy);
	free(cfg->ssl_service_key);
	free(cfg->ssl_service_pem);
	free(cfg->tls_cert_bundle);
	config_delstrlist(cfg->tls_additional_port);
	config_delstrlist(cfg->tls_session_ticket_keys.first);
	free(cfg->tls_ciphers);
	free(cfg->tls_ciphersuites);
	free(cfg->http_endpoint);
	if(cfg->log_identity) {
		log_ident_revert_to_default();
		free(cfg->log_identity);
	}
	config_del_strarray(cfg->ifs, cfg->num_ifs);
	config_del_strarray(cfg->out_ifs, cfg->num_out_ifs);
	config_delstubs(cfg->stubs);
	config_delstubs(cfg->forwards);
	config_delauths(cfg->auths);
	config_delviews(cfg->views);
	config_delstrlist(cfg->donotqueryaddrs);
	config_delstrlist(cfg->root_hints);
#ifdef CLIENT_SUBNET
	config_delstrlist(cfg->client_subnet);
	config_delstrlist(cfg->client_subnet_zone);
#endif
	free(cfg->identity);
	free(cfg->version);
	free(cfg->http_user_agent);
	free(cfg->nsid_cfg_str);
	free(cfg->nsid);
	free(cfg->module_conf);
	free(cfg->outgoing_avail_ports);
	config_delstrlist(cfg->caps_whitelist);
	config_delstrlist(cfg->private_address);
	config_delstrlist(cfg->private_domain);
	config_delstrlist(cfg->auto_trust_anchor_file_list);
	config_delstrlist(cfg->trust_anchor_file_list);
	config_delstrlist(cfg->trusted_keys_file_list);
	config_delstrlist(cfg->trust_anchor_list);
	config_delstrlist(cfg->domain_insecure);
	config_deldblstrlist(cfg->acls);
	config_deldblstrlist(cfg->tcp_connection_limits);
	free(cfg->val_nsec3_key_iterations);
	config_deldblstrlist(cfg->local_zones);
	config_delstrlist(cfg->local_zones_nodefault);
#ifdef USE_IPSET
	config_delstrlist(cfg->local_zones_ipset);
#endif
	config_delstrlist(cfg->local_data);
	config_deltrplstrlist(cfg->local_zone_overrides);
	config_del_strarray(cfg->tagname, cfg->num_tags);
	config_del_strbytelist(cfg->local_zone_tags);
	config_del_strbytelist(cfg->respip_tags);
	config_deldblstrlist(cfg->acl_view);
	config_del_strbytelist(cfg->acl_tags);
	config_deltrplstrlist(cfg->acl_tag_actions);
	config_deltrplstrlist(cfg->acl_tag_datas);
	config_deldblstrlist(cfg->interface_actions);
	config_deldblstrlist(cfg->interface_view);
	config_del_strbytelist(cfg->interface_tags);
	config_deltrplstrlist(cfg->interface_tag_actions);
	config_deltrplstrlist(cfg->interface_tag_datas);
	config_delstrlist(cfg->control_ifs.first);
	config_deldblstrlist(cfg->wait_limit_netblock);
	config_deldblstrlist(cfg->wait_limit_cookie_netblock);
	free(cfg->server_key_file);
	free(cfg->server_cert_file);
	free(cfg->control_key_file);
	free(cfg->control_cert_file);
	free(cfg->nat64_prefix);
	free(cfg->dns64_prefix);
	config_delstrlist(cfg->dns64_ignore_aaaa);
	free(cfg->dnstap_socket_path);
	free(cfg->dnstap_ip);
	free(cfg->dnstap_tls_server_name);
	free(cfg->dnstap_tls_cert_bundle);
	free(cfg->dnstap_tls_client_key_file);
	free(cfg->dnstap_tls_client_cert_file);
	free(cfg->dnstap_identity);
	free(cfg->dnstap_version);
	config_deldblstrlist(cfg->ratelimit_for_domain);
	config_deldblstrlist(cfg->ratelimit_below_domain);
	config_delstrlist(cfg->python_script);
	config_delstrlist(cfg->dynlib_file);
	config_deldblstrlist(cfg->edns_client_strings);
	config_delstrlist(cfg->proxy_protocol_port);
#ifdef USE_IPSECMOD
	free(cfg->ipsecmod_hook);
	config_delstrlist(cfg->ipsecmod_whitelist);
#endif
#ifdef USE_CACHEDB
	free(cfg->cachedb_backend);
	free(cfg->cachedb_secret);
#ifdef USE_REDIS
	free(cfg->redis_server_host);
	free(cfg->redis_server_path);
	free(cfg->redis_server_password);
#endif  /* USE_REDIS */
#endif  /* USE_CACHEDB */
#ifdef USE_IPSET
	free(cfg->ipset_name_v4);
	free(cfg->ipset_name_v6);
#endif
	free(cfg);
}