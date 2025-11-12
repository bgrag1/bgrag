ESP_SYSTEM_INIT_FN(init_secure, CORE, BIT(0), 150)
{
#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
    // For anti-rollback case, recheck security version before we boot-up the current application
    const esp_app_desc_t *desc = esp_app_get_description();
    ESP_RETURN_ON_FALSE(esp_efuse_check_secure_version(desc->secure_version), ESP_FAIL, TAG, "Incorrect secure version of app");
#endif

#ifdef CONFIG_SECURE_FLASH_ENC_ENABLED
    esp_flash_encryption_init_checks();
#endif

#if defined(CONFIG_SECURE_BOOT) || defined(CONFIG_SECURE_SIGNED_ON_UPDATE_NO_SECURE_BOOT)
    // Note: in some configs this may read flash, so placed after flash init
    esp_secure_boot_init_checks();
#endif

#if SOC_EFUSE_ECDSA_USE_HARDWARE_K
    if (esp_efuse_find_purpose(ESP_EFUSE_KEY_PURPOSE_ECDSA_KEY, NULL)) {
        // ECDSA key purpose block is present and hence permanently enable
        // the hardware TRNG supplied k mode (most secure mode)
        ESP_RETURN_ON_ERROR(esp_efuse_write_field_bit(ESP_EFUSE_ECDSA_FORCE_USE_HARDWARE_K), TAG, "Failed to enable hardware k mode");
    }
#endif

#if CONFIG_SECURE_DISABLE_ROM_DL_MODE
    ESP_RETURN_ON_ERROR(esp_efuse_disable_rom_download_mode(), TAG, "Failed to disable ROM download mode");
#endif

#if CONFIG_SECURE_ENABLE_SECURE_ROM_DL_MODE
    ESP_RETURN_ON_ERROR(esp_efuse_enable_rom_secure_download_mode(), TAG, "Failed to enable Secure Download mode");
#endif

#if CONFIG_ESP32_DISABLE_BASIC_ROM_CONSOLE
    esp_efuse_disable_basic_rom_console();
#endif
    return ESP_OK;
}