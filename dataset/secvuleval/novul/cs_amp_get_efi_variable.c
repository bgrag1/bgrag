static efi_status_t cs_amp_get_efi_variable(efi_char16_t *name,
					    efi_guid_t *guid,
					    unsigned long *size,
					    void *buf)
{
	u32 attr;

	KUNIT_STATIC_STUB_REDIRECT(cs_amp_get_efi_variable, name, guid, size, buf);

	if (efi_rt_services_supported(EFI_RT_SUPPORTED_GET_VARIABLE))
		return efi.get_variable(name, guid, &attr, size, buf);

	return EFI_NOT_FOUND;
}
