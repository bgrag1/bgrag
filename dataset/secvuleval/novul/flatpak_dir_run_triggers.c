flatpak_dir_run_triggers (FlatpakDir   *self,
                          GCancellable *cancellable,
                          GError      **error)
{
  gboolean ret = FALSE;
  g_autoptr(GFileEnumerator) dir_enum = NULL;
  g_autoptr(GFileInfo) child_info = NULL;
  g_autoptr(GFile) triggersdir = NULL;
  GError *temp_error = NULL;
  const char *triggerspath;

  maybe_reload_dbus_config (cancellable);

  if (flatpak_dir_use_system_helper (self, NULL))
    {
      const char *installation = flatpak_dir_get_id (self);

      if (!flatpak_dir_system_helper_call_run_triggers (self,
                                                        FLATPAK_HELPER_RUN_TRIGGERS_FLAGS_NONE,
                                                        installation ? installation : "",
                                                        cancellable,
                                                        error))
        return FALSE;

      return TRUE;
    }

  triggerspath = g_getenv ("FLATPAK_TRIGGERSDIR");
  if (triggerspath == NULL)
    triggerspath = FLATPAK_TRIGGERDIR;

  g_info ("running triggers from %s", triggerspath);

  triggersdir = g_file_new_for_path (triggerspath);

  dir_enum = g_file_enumerate_children (triggersdir, "standard::type,standard::name",
                                        0, cancellable, error);
  if (!dir_enum)
    goto out;

  while ((child_info = g_file_enumerator_next_file (dir_enum, cancellable, &temp_error)) != NULL)
    {
      g_autoptr(GFile) child = NULL;
      const char *name;
      GError *trigger_error = NULL;

      name = g_file_info_get_name (child_info);

      child = g_file_get_child (triggersdir, name);

      if (g_file_info_get_file_type (child_info) == G_FILE_TYPE_REGULAR &&
          g_str_has_suffix (name, ".trigger"))
        {
          /* We need to canonicalize the basedir, because if has a symlink
             somewhere the bind mount will be on the target of that, not
             at that exact path. */
          g_autofree char *basedir_orig = g_file_get_path (self->basedir);
          g_autofree char *basedir = realpath (basedir_orig, NULL);
          g_autoptr(FlatpakBwrap) bwrap = NULL;
          g_autofree char *commandline = NULL;

          g_info ("running trigger %s", name);

          bwrap = flatpak_bwrap_new (NULL);

#ifndef DISABLE_SANDBOXED_TRIGGERS
          flatpak_bwrap_add_arg (bwrap, flatpak_get_bwrap ());
          flatpak_bwrap_add_args (bwrap,
                                  "--unshare-ipc",
                                  "--unshare-net",
                                  "--unshare-pid",
                                  "--ro-bind", "/", "/",
                                  "--proc", "/proc",
                                  "--dev", "/dev",
                                  "--bind", basedir, basedir,
                                  "--",
                                  NULL);
#endif
          flatpak_bwrap_add_args (bwrap,
                                  flatpak_file_get_path_cached (child),
                                  basedir,
                                  NULL);
          flatpak_bwrap_finish (bwrap);

          commandline = flatpak_quote_argv ((const char **) bwrap->argv->pdata, -1);
          g_info ("Running '%s'", commandline);

          /* We use LEAVE_DESCRIPTORS_OPEN and close them in the child_setup
           * to work around a deadlock in GLib < 2.60 */
          if (!g_spawn_sync ("/",
                             (char **) bwrap->argv->pdata,
                             NULL,
                             G_SPAWN_SEARCH_PATH | G_SPAWN_LEAVE_DESCRIPTORS_OPEN,
                             flatpak_bwrap_child_setup_cb, bwrap->fds,
                             NULL, NULL,
                             NULL, &trigger_error))
            {
              g_warning ("Error running trigger %s: %s", name, trigger_error->message);
              g_clear_error (&trigger_error);
            }
        }

      g_clear_object (&child_info);
    }

  if (temp_error != NULL)
    {
      g_propagate_error (error, temp_error);
      goto out;
    }

  ret = TRUE;
out:
  return ret;
}