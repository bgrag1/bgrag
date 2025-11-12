int app_initialize(const char *const argv[])
{
    const char *argv_out[OGS_ARG_MAX];
    bool user_config = false;
    int i = 0;

    for (i = 0; argv[i] && i < OGS_ARG_MAX-3; i++) {
        if (strcmp("-c", argv[i]) == 0) {
            user_config = true;
        }
        argv_out[i] = argv[i];
    }
    argv_out[i] = NULL;

    if (!user_config) {
        argv_out[i++] = "-c";
        argv_out[i++] = DEFAULT_CONFIG_FILENAME;
        argv_out[i] = NULL;
    }

    if (ogs_app()->parameter.no_nrf == 0)
        nrf_thread = test_child_create("nrf", argv_out);
    if (ogs_app()->parameter.no_scp == 0)
        scp_thread = test_child_create("scp", argv_out);
    if (ogs_app()->parameter.no_hss == 0)
        hss_thread = test_child_create("hss", argv_out);
    if (ogs_app()->parameter.no_pcrf == 0)
        pcrf_thread = test_child_create("pcrf", argv_out);

    if (ogs_app()->parameter.no_upf == 0)
        upf_thread = test_child_create("upf", argv_out);
    if (ogs_app()->parameter.no_sgwu == 0)
        sgwu_thread = test_child_create("sgwu", argv_out);

    if (ogs_app()->parameter.no_smf == 0)
        smf_thread = test_child_create("smf", argv_out);
    if (ogs_app()->parameter.no_sgwc == 0)
        sgwc_thread = test_child_create("sgwc", argv_out);

    if (ogs_app()->parameter.no_mme == 0)
        mme_thread = test_child_create("mme", argv_out);
    if (ogs_app()->parameter.no_amf == 0)
        amf_thread = test_child_create("amf", argv_out);

    if (ogs_app()->parameter.no_ausf == 0)
        ausf_thread = test_child_create("ausf", argv_out);
    if (ogs_app()->parameter.no_udm == 0)
        udm_thread = test_child_create("udm", argv_out);
    if (ogs_app()->parameter.no_pcf == 0)
        pcf_thread = test_child_create("pcf", argv_out);
    if (ogs_app()->parameter.no_nssf == 0)
        nssf_thread = test_child_create("nssf", argv_out);
    if (ogs_app()->parameter.no_bsf == 0)
        bsf_thread = test_child_create("bsf", argv_out);
    if (ogs_app()->parameter.no_udr == 0)
        udr_thread = test_child_create("udr", argv_out);

    /*
     * Wait for all sockets listening
     *
     * Note that at least 4 seconds are needed if freeDiameter is running.
     */
    ogs_msleep(5000);

    return OGS_OK;;
}