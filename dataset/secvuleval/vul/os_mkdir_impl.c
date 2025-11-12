os_mkdir_impl(PyObject *module, path_t *path, int mode, int dir_fd)
/*[clinic end generated code: output=a70446903abe821f input=a61722e1576fab03]*/
{
    int result;
#ifdef HAVE_MKDIRAT
    int mkdirat_unavailable = 0;
#endif

    if (PySys_Audit("os.mkdir", "Oii", path->object, mode,
                    dir_fd == DEFAULT_DIR_FD ? -1 : dir_fd) < 0) {
        return NULL;
    }

#ifdef MS_WINDOWS
    Py_BEGIN_ALLOW_THREADS
    result = CreateDirectoryW(path->wide, NULL);
    Py_END_ALLOW_THREADS

    if (!result)
        return path_error(path);
#else
    Py_BEGIN_ALLOW_THREADS
#if HAVE_MKDIRAT
    if (dir_fd != DEFAULT_DIR_FD) {
      if (HAVE_MKDIRAT_RUNTIME) {
        result = mkdirat(dir_fd, path->narrow, mode);

      } else {
        mkdirat_unavailable = 1;
      }
    } else
#endif
#if defined(__WATCOMC__) && !defined(__QNX__)
        result = mkdir(path->narrow);
#else
        result = mkdir(path->narrow, mode);
#endif
    Py_END_ALLOW_THREADS

#if HAVE_MKDIRAT
    if (mkdirat_unavailable) {
        argument_unavailable_error(NULL, "dir_fd");
        return NULL;
    }
#endif

    if (result < 0)
        return path_error(path);
#endif /* MS_WINDOWS */
    Py_RETURN_NONE;
}