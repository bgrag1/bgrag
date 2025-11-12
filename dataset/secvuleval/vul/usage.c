static void usage(const char *argv0) {
    printf("usage:\t%s /path/to/wwwroot [flags]\n\n", argv0);
    printf("flags:\t--port number (default: %u, or 80 if running as root)\n"
    "\t\tSpecifies which port to listen on for connections.\n"
    "\t\tPass 0 to let the system choose any free port for you.\n\n", bindport);
    printf("\t--addr ip (default: all)\n"
    "\t\tIf multiple interfaces are present, specifies\n"
    "\t\twhich one to bind the listening port to.\n\n");
    printf("\t--maxconn number (default: system maximum)\n"
    "\t\tSpecifies how many concurrent connections to accept.\n\n");
    printf("\t--log filename (default: stdout)\n"
    "\t\tSpecifies which file to append the request log to.\n\n");
    printf("\t--syslog\n"
    "\t\tUse syslog for request log.\n\n");
    printf("\t--chroot (default: don't chroot)\n"
    "\t\tLocks server into wwwroot directory for added security.\n\n");
    printf("\t--daemon (default: don't daemonize)\n"
    "\t\tDetach from the controlling terminal and run in the background.\n\n");
    printf("\t--index filename (default: %s)\n"
    "\t\tDefault file to serve when a directory is requested.\n\n",
        index_name);
    printf("\t--no-listing\n"
    "\t\tDo not serve listing if directory is requested.\n\n");
    printf("\t--mimetypes filename (optional)\n"
    "\t\tParses specified file for extension-MIME associations.\n\n");
    printf("\t--default-mimetype string (optional, default: %s)\n"
    "\t\tFiles with unknown extensions are served as this mimetype.\n\n",
        octet_stream);
    printf("\t--uid uid/uname, --gid gid/gname (default: don't privdrop)\n"
    "\t\tDrops privileges to given uid:gid after initialization.\n\n");
    printf("\t--pidfile filename (default: no pidfile)\n"
    "\t\tWrite PID to the specified file.  Note that if you are\n"
    "\t\tusing --chroot, then the pidfile must be relative to,\n"
    "\t\tand inside the wwwroot.\n\n");
    printf("\t--no-keepalive\n"
    "\t\tDisables HTTP Keep-Alive functionality.\n\n");
#ifdef __FreeBSD__
    printf("\t--accf (default: don't use acceptfilter)\n"
    "\t\tUse acceptfilter.  Needs the accf_http module loaded.\n\n");
#endif
    printf("\t--forward host url (default: don't forward)\n"
    "\t\tWeb forward (301 redirect).\n"
    "\t\tRequests to the host are redirected to the corresponding url.\n"
    "\t\tThe option may be specified multiple times, in which case\n"
    "\t\tthe host is matched in order of appearance.\n\n");
    printf("\t--forward-all url (default: don't forward)\n"
    "\t\tWeb forward (301 redirect).\n"
    "\t\tAll requests are redirected to the corresponding url.\n\n");
    printf("\t--no-server-id\n"
    "\t\tDon't identify the server type in headers\n"
    "\t\tor directory listings.\n\n");
    printf("\t--timeout secs (default: %d)\n"
    "\t\tIf a connection is idle for more than this many seconds,\n"
    "\t\tit will be closed. Set to zero to disable timeouts.\n\n",
    timeout_secs);
    printf("\t--auth username:password\n"
    "\t\tEnable basic authentication.\n\n");
    printf("\t--forward-https\n"
    "\t\tIf the client requested HTTP, forward to HTTPS.\n"
    "\t\tThis is useful if darkhttpd is behind a reverse proxy\n"
    "\t\tthat supports SSL.\n\n");
    printf("\t--header 'Header: Value'\n"
    "\t\tAdd a custom header to all responses.\n"
    "\t\tThis option can be specified multiple times, in which case\n"
    "\t\tthe headers are added in order of appearance.\n\n");
#ifdef HAVE_INET6
    printf("\t--ipv6\n"
    "\t\tListen on IPv6 address.\n\n");
#else
    printf("\t(This binary was built without IPv6 support: -DNO_IPV6)\n\n");
#endif
}