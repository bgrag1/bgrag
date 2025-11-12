static esp_err_t image_load(esp_image_load_mode_t mode, const esp_partition_pos_t *part, esp_image_metadata_t *data)
{
#ifdef BOOTLOADER_BUILD
    bool do_load   = (mode == ESP_IMAGE_LOAD) || (mode == ESP_IMAGE_LOAD_NO_VALIDATE);
    bool do_verify = (mode == ESP_IMAGE_LOAD) || (mode == ESP_IMAGE_VERIFY) || (mode == ESP_IMAGE_VERIFY_SILENT);
#else
    bool do_load   = false; // Can't load the image in app mode
    bool do_verify = true;  // In app mode is available only verify mode
#endif
    bool silent    = (mode == ESP_IMAGE_VERIFY_SILENT);
    esp_err_t err = ESP_OK;
    // checksum the image a word at a time. This shaves 30-40ms per MB of image size
    uint32_t checksum_word = ESP_ROM_CHECKSUM_INITIAL;
    uint32_t *checksum = (do_verify) ? &checksum_word : NULL;
    bootloader_sha256_handle_t sha_handle = NULL;
    bool verify_sha;
#if (SECURE_BOOT_CHECK_SIGNATURE == 1)
     /* used for anti-FI checks */
    uint8_t image_digest[HASH_LEN] = { [ 0 ... 31] = 0xEE };
    uint8_t verified_digest[HASH_LEN] = { [ 0 ... 31 ] = 0x01 };
#endif

    if (data == NULL || part == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

#if CONFIG_SECURE_BOOT_V2_ENABLED
    // For Secure Boot V2, we do verify signature on bootloader which includes the SHA calculation.
    verify_sha = do_verify;
#else // Secure boot not enabled
    // For secure boot V1 on ESP32, we don't calculate SHA or verify signature on bootloaders.
    // (For non-secure boot, we don't verify any SHA-256 hash appended to the bootloader because
    // esptool.py may have rewritten the header - rely on esptool.py having verified the bootloader at flashing time, instead.)
    verify_sha = (part->offset != ESP_BOOTLOADER_OFFSET) && do_verify;
#endif

    if (part->size > SIXTEEN_MB) {
        err = ESP_ERR_INVALID_ARG;
        FAIL_LOAD("partition size 0x%"PRIx32" invalid, larger than 16MB", part->size);
    }

    bootloader_sha256_handle_t *p_sha_handle = &sha_handle;
    CHECK_ERR(process_image_header(data, part->offset, (verify_sha) ? p_sha_handle : NULL, do_verify, silent));
    CHECK_ERR(process_segments(data, silent, do_load, sha_handle, checksum));
    bool skip_check_checksum = !do_verify || esp_cpu_dbgr_is_attached();
    CHECK_ERR(process_checksum(sha_handle, checksum_word, data, silent, skip_check_checksum));
    CHECK_ERR(process_appended_hash_and_sig(data, part->offset, part->size, do_verify, silent));
    if (verify_sha) {
#if (SECURE_BOOT_CHECK_SIGNATURE == 1)
        // secure boot images have a signature appended
#if defined(BOOTLOADER_BUILD) && !defined(CONFIG_SECURE_BOOT)
        // If secure boot is not enabled in hardware, then
        // skip the signature check in bootloader when the debugger is attached.
        // This is done to allow for breakpoints in Flash.
        bool do_verify_sig = !esp_cpu_dbgr_is_attached();
#else // CONFIG_SECURE_BOOT
        bool do_verify_sig = true;
#endif // end checking for JTAG
        if (do_verify_sig) {
            err = verify_secure_boot_signature(sha_handle, data, image_digest, verified_digest);
            sha_handle = NULL; // verify_secure_boot_signature finishes sha_handle
        }
#else // SECURE_BOOT_CHECK_SIGNATURE
        // No secure boot, but SHA-256 can be appended for basic corruption detection
        if (sha_handle != NULL && !esp_cpu_dbgr_is_attached()) {
            err = verify_simple_hash(sha_handle, data);
            sha_handle = NULL; // calling verify_simple_hash finishes sha_handle
        }
#endif // SECURE_BOOT_CHECK_SIGNATURE
    } // verify_sha

    // bootloader may still have a sha256 digest handle open
    if (sha_handle != NULL) {
        bootloader_sha256_finish(sha_handle, NULL);
        sha_handle = NULL;
    }

    if (err != ESP_OK) {
        goto err;
    }

#ifdef BOOTLOADER_BUILD

#if (SECURE_BOOT_CHECK_SIGNATURE == 1)
    /* If signature was checked in bootloader build, verified_digest should equal image_digest

       This is to detect any fault injection that caused signature verification to not complete normally.

       Any attack which bypasses this check should be of limited use as the RAM contents are still obfuscated, therefore we do the check
       immediately before we deobfuscate.

       Note: the conditions for making this check are the same as for setting verify_sha above, but on ESP32 SB V1 we move the test for
       "only verify signature in bootloader" into the macro so it's tested multiple times.
     */
#if CONFIG_SECURE_BOOT_V2_ENABLED
    ESP_FAULT_ASSERT(!esp_secure_boot_enabled() || memcmp(image_digest, verified_digest, HASH_LEN) == 0);
#else // Secure Boot V1 on ESP32, only verify signatures for apps not bootloaders
    ESP_FAULT_ASSERT(data->start_addr == ESP_BOOTLOADER_OFFSET || memcmp(image_digest, verified_digest, HASH_LEN) == 0);
#endif

#endif // SECURE_BOOT_CHECK_SIGNATURE

    // Deobfuscate RAM
    if (do_load && ram_obfs_value[0] != 0 && ram_obfs_value[1] != 0) {
        for (int i = 0; i < data->image.segment_count; i++) {
            uint32_t load_addr = data->segments[i].load_addr;
            if (should_load(load_addr)) {
                uint32_t *loaded = (uint32_t *)load_addr;
                for (size_t j = 0; j < data->segments[i].data_len / sizeof(uint32_t); j++) {
                    loaded[j] ^= (j & 1) ? ram_obfs_value[0] : ram_obfs_value[1];
                }
            }
        }
#if SOC_CACHE_INTERNAL_MEM_VIA_L1CACHE
        cache_ll_writeback_all(CACHE_LL_LEVEL_INT_MEM, CACHE_TYPE_DATA, CACHE_LL_ID_ALL);
#endif
    }

#if CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
    /* For anti-rollback case, reconfirm security version of the application to prevent FI attacks */
    bool sec_ver = false;
    if (do_load) {
        sec_ver = esp_efuse_check_secure_version(data->secure_version);
        if (!sec_ver) {
            err = ESP_FAIL;
            goto err;
        }
    }
    /* Ensure that the security version check passes for image loading scenario */
    ESP_FAULT_ASSERT(!do_load || sec_ver == true);
#endif // CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK

#endif // BOOTLOADER_BUILD

    // Success!
    return ESP_OK;

err:
    if (err == ESP_OK) {
        err = ESP_ERR_IMAGE_INVALID;
    }
    if (sha_handle != NULL) {
        // Need to finish the hash process to free the handle
        bootloader_sha256_finish(sha_handle, NULL);
    }
    // Prevent invalid/incomplete data leaking out
    bzero(data, sizeof(esp_image_metadata_t));
    return err;
}