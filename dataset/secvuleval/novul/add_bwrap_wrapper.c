add_bwrap_wrapper (FlatpakBwrap *bwrap,
                   const char   *app_info_path,
                   GError      **error)
{
  glnx_autofd int app_info_fd = -1;
  g_auto(GLnxDirFdIterator) dir_iter = { 0 };
  struct dirent *dent;
  g_autofree char *user_runtime_dir = flatpak_get_real_xdg_runtime_dir ();
  g_autofree char *proxy_socket_dir = g_build_filename (user_runtime_dir, ".dbus-proxy/", NULL);

  app_info_fd = open (app_info_path, O_RDONLY | O_CLOEXEC);
  if (app_info_fd == -1)
    return glnx_throw_errno_prefix (error, _("Failed to open app info file"));

  if (!glnx_dirfd_iterator_init_at (AT_FDCWD, "/", FALSE, &dir_iter, error))
    return FALSE;

  flatpak_bwrap_add_arg (bwrap, flatpak_get_bwrap ());

  while (TRUE)
    {
      glnx_autofd int o_path_fd = -1;
      struct statfs stfs;

      if (!glnx_dirfd_iterator_next_dent_ensure_dtype (&dir_iter, &dent, NULL, error))
        return FALSE;

      if (dent == NULL)
        break;

      if (strcmp (dent->d_name, ".flatpak-info") == 0)
        continue;

      /* O_PATH + fstatfs is the magic that we need to statfs without automounting the target */
      o_path_fd = openat (dir_iter.fd, dent->d_name, O_PATH | O_NOFOLLOW | O_CLOEXEC);
      if (o_path_fd == -1 || fstatfs (o_path_fd, &stfs) != 0 || stfs.f_type == AUTOFS_SUPER_MAGIC)
        continue; /* AUTOFS mounts are risky and can cause us to block (see issue #1633), so ignore it. Its unlikely the proxy needs such a directory. */

      if (dent->d_type == DT_DIR)
        {
          if (strcmp (dent->d_name, "tmp") == 0 ||
              strcmp (dent->d_name, "var") == 0 ||
              strcmp (dent->d_name, "run") == 0)
            flatpak_bwrap_add_arg (bwrap, "--bind");
          else
            flatpak_bwrap_add_arg (bwrap, "--ro-bind");

          flatpak_bwrap_add_arg_printf (bwrap, "/%s", dent->d_name);
          flatpak_bwrap_add_arg_printf (bwrap, "/%s", dent->d_name);
        }
      else if (dent->d_type == DT_LNK)
        {
          g_autofree gchar *target = NULL;

          target = glnx_readlinkat_malloc (dir_iter.fd, dent->d_name,
                                           NULL, error);
          if (target == NULL)
            return FALSE;
          flatpak_bwrap_add_args (bwrap, "--symlink", target, NULL);
          flatpak_bwrap_add_arg_printf (bwrap, "/%s", dent->d_name);
        }
    }

  flatpak_bwrap_add_args (bwrap, "--bind", proxy_socket_dir, proxy_socket_dir, NULL);

  /* This is a file rather than a bind mount, because it will then
     not be unmounted from the namespace when the namespace dies. */
  flatpak_bwrap_add_args (bwrap, "--perms", "0600", NULL);
  flatpak_bwrap_add_args_data_fd (bwrap, "--file", g_steal_fd (&app_info_fd), "/.flatpak-info");

  if (!flatpak_bwrap_bundle_args (bwrap, 1, -1, FALSE, error))
    return FALSE;

  /* End of options: the next argument will be the executable name */
  flatpak_bwrap_add_arg (bwrap, "--");

  return TRUE;
}