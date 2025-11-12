RunTerminationHelper(HANDLE console_token, DWORD pid) {
  WCHAR module_path[MAX_PATH];
  GetModuleFileNameW(NULL, module_path, _countof(module_path));
  std::wstring command { module_path };

  command += L" --terminate " + std::to_wstring(pid);

  STARTUPINFOW startup_info = {};
  startup_info.cb = sizeof(startup_info);
  startup_info.lpDesktop = (LPWSTR) L"winsta0\\default";

  // Execute ourselves as a detached process in the user session with the --terminate argument.
  // This will allow us to attach to Sunshine's console and send it a Ctrl-C event.
  PROCESS_INFORMATION process_info;
  if (!CreateProcessAsUserW(console_token,
        NULL,
        (LPWSTR) command.c_str(),
        NULL,
        NULL,
        FALSE,
        CREATE_UNICODE_ENVIRONMENT | DETACHED_PROCESS,
        NULL,
        NULL,
        &startup_info,
        &process_info)) {
    return false;
  }

  // Wait for the termination helper to complete
  WaitForSingleObject(process_info.hProcess, INFINITE);

  // Check the exit status of the helper process
  DWORD exit_code;
  GetExitCodeProcess(process_info.hProcess, &exit_code);

  // Cleanup handles
  CloseHandle(process_info.hProcess);
  CloseHandle(process_info.hThread);

  // If the helper process returned 0, it succeeded
  return exit_code == 0;
}