static void run(int argc, const char *const argv[],
        const char *name, void (*init)(const char * const argv[]))
{
    int rv;
    bool user_config;

    /* '-f sample-XXXX.conf -e error' is always added */
    const char *argv_out[argc+5], *new_argv[argc+5];
    int argc_out;

    char conf_file[OGS_MAX_FILEPATH_LEN];
    
    user_config = false;
    for (argc_out = 0; argc_out < argc; argc_out++) {
        if (strcmp("-c", argv[argc_out]) == 0) {
            user_config = true; 
        }
        argv_out[argc_out] = argv[argc_out];
    }
    argv_out[argc_out] = NULL;

    if (!user_config) {
        /* buildroot/configs/XXX-conf.yaml */
        ogs_snprintf(conf_file, sizeof conf_file, "%s%s",
            MESON_BUILD_ROOT OGS_DIR_SEPARATOR_S
            "configs" OGS_DIR_SEPARATOR_S, name);
        argv_out[argc_out++] = "-c";
        argv_out[argc_out++] = conf_file;
        argv_out[argc_out] = NULL;
    }

    /* buildroot/src/open5gs-main */
    argv_out[0] = MESON_BUILD_ROOT OGS_DIR_SEPARATOR_S 
            "src" OGS_DIR_SEPARATOR_S "open5gs-main";

    rv = abts_main(argc_out, argv_out, new_argv);
    ogs_assert(rv == OGS_OK);

    (*init)(new_argv);
}