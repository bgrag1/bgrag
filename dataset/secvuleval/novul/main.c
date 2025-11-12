int main(int argc, const char *const argv[])
{
    /**************************************************************************
     * Starting up process.
     *
     * Keep the order of starting-up
     */
    int rv, i, opt;
    ogs_getopt_t options;
    struct {
        char *config_file;
        char *log_file;
        char *log_level;
        char *domain_mask;

        bool enable_debug;
        bool enable_trace;
    } optarg;
    const char *argv_out[argc+1];

    memset(&optarg, 0, sizeof(optarg));

    ogs_getopt_init(&options, (char**)argv);
    while ((opt = ogs_getopt(&options, "vhDc:l:e:m:dt")) != -1) {
        switch (opt) {
        case 'v':
            show_version();
            return OGS_OK;
        case 'h':
            show_help(argv[0]);
            return OGS_OK;
        case 'D':
#if !defined(_WIN32)
        {
            pid_t pid;
            pid = fork();

            ogs_assert(pid >= 0);

            if (pid != 0)
            {
                /* Parent */
                return EXIT_SUCCESS;
            }
            /* Child */

            setsid();
            umask(027);
        }
#else
            printf("%s: Not Support in WINDOWS", argv[0]);
#endif
            break;
        case 'c':
            optarg.config_file = options.optarg;
            break;
        case 'l':
            optarg.log_file = options.optarg;
            break;
        case 'e':
            optarg.log_level = options.optarg;
            break;
        case 'm':
            optarg.domain_mask = options.optarg;
            break;
        case 'd':
            optarg.enable_debug = true;
            break;
        case 't':
            optarg.enable_trace = true;
            break;
        case '?':
            fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
            show_help(argv[0]);
            return OGS_ERROR;
        default:
            fprintf(stderr, "%s: should not be reached\n", OGS_FUNC);
            return OGS_ERROR;
        }
    }

    if (optarg.enable_debug) optarg.log_level = (char*)"debug";
    if (optarg.enable_trace) optarg.log_level = (char*)"trace";

    i = 0;
    argv_out[i++] = argv[0];

    if (optarg.config_file) {
        argv_out[i++] = "-c";
        argv_out[i++] = optarg.config_file;
    }
    if (optarg.log_file) {
        argv_out[i++] = "-l";
        argv_out[i++] = optarg.log_file;
    }
    if (optarg.log_level) {
        argv_out[i++] = "-e";
        argv_out[i++] = optarg.log_level;
    }
    if (optarg.domain_mask) {
        argv_out[i++] = "-m";
        argv_out[i++] = optarg.domain_mask;
    }

    argv_out[i] = NULL;

    ogs_signal_init();
    ogs_setup_signal_thread();

    rv = ogs_app_initialize(OPEN5GS_VERSION, DEFAULT_CONFIG_FILENAME, argv_out);
    if (rv != OGS_OK) {
        if (rv == OGS_RETRY)
            return EXIT_SUCCESS;

        ogs_fatal("Open5GS initialization failed. Aborted");
        return OGS_ERROR;
    }

    rv = app_initialize(argv_out);
    if (rv != OGS_OK) {
        if (rv == OGS_RETRY)
            return EXIT_SUCCESS;

        ogs_fatal("Open5GS initialization failed. Aborted");
        return OGS_ERROR;
    }

    atexit(terminate);
    ogs_signal_thread(check_signal);

    ogs_info("Open5GS daemon terminating...");

    return OGS_OK;
}