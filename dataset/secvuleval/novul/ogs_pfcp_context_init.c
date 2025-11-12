void ogs_pfcp_context_init(void)
{
    int i;
    ogs_assert(context_initialized == 0);

    /* Initialize SMF context */
    memset(&self, 0, sizeof(ogs_pfcp_context_t));

    self.local_recovery = ogs_time_ntp32_now();

    ogs_log_install_domain(&__ogs_pfcp_domain, "pfcp", ogs_core()->log.level);

    ogs_pool_init(&ogs_pfcp_node_pool, ogs_app()->pool.nf);

    ogs_pool_init(&ogs_pfcp_sess_pool, ogs_app()->pool.sess);

    ogs_pool_init(&ogs_pfcp_far_pool,
            ogs_app()->pool.sess * OGS_MAX_NUM_OF_FAR);
    ogs_pool_init(&ogs_pfcp_urr_pool,
            ogs_app()->pool.sess * OGS_MAX_NUM_OF_URR);
    ogs_pool_init(&ogs_pfcp_qer_pool,
            ogs_app()->pool.sess * OGS_MAX_NUM_OF_QER);
    ogs_pool_init(&ogs_pfcp_bar_pool,
            ogs_app()->pool.sess * OGS_MAX_NUM_OF_BAR);

    ogs_pool_init(&ogs_pfcp_pdr_pool,
            ogs_app()->pool.sess * OGS_MAX_NUM_OF_PDR);
    ogs_pool_init(&ogs_pfcp_pdr_teid_pool, ogs_pfcp_pdr_pool.size);
    ogs_pool_random_id_generate(&ogs_pfcp_pdr_teid_pool);

    pdr_random_to_index = ogs_calloc(
            sizeof(ogs_pool_id_t), ogs_pfcp_pdr_pool.size+1);
    ogs_assert(pdr_random_to_index);
    for (i = 0; i < ogs_pfcp_pdr_pool.size; i++)
        pdr_random_to_index[ogs_pfcp_pdr_teid_pool.array[i]] = i;

    ogs_pool_init(&ogs_pfcp_rule_pool,
            ogs_app()->pool.sess *
            OGS_MAX_NUM_OF_PDR * OGS_MAX_NUM_OF_FLOW_IN_PDR);

    ogs_pool_init(&ogs_pfcp_dev_pool, OGS_MAX_NUM_OF_DEV);
    ogs_pool_init(&ogs_pfcp_subnet_pool, OGS_MAX_NUM_OF_SUBNET);

    self.object_teid_hash = ogs_hash_make();
    ogs_assert(self.object_teid_hash);
    self.far_f_teid_hash = ogs_hash_make();
    ogs_assert(self.far_f_teid_hash);
    self.far_teid_hash = ogs_hash_make();
    ogs_assert(self.far_teid_hash);

    context_initialized = 1;
}