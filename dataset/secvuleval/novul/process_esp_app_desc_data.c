static size_t process_esp_app_desc_data(const uint32_t *src, bootloader_sha256_handle_t sha_handle, uint32_t *checksum, esp_image_metadata_t *metadata)
{
    /* Using data_buffer here helps to securely read secure_version
     * (for anti-rollback) from esp_app_desc_t, preventing FI attack.
     * We read data from Flash into this buffer, which is covered by sha256.
     * Therefore, if the flash is under attackers control and contents are modified
     * the sha256 comparison will fail.
     *
     * The esp_app_desc_t structure is located in DROM and is always in segment #0.
     *
     * esp_app_desc_t is always at #0 segment (index==0).
     * secure_version field of esp_app_desc_t is located at #2 word (w_i==1).
     */
    uint32_t data_buffer[2];
    memcpy(data_buffer, src, sizeof(data_buffer));
    assert(data_buffer[0] == ESP_APP_DESC_MAGIC_WORD);
    metadata->secure_version = data_buffer[1];
    if (checksum != NULL) {
        *checksum ^= data_buffer[0] ^ data_buffer[1];
    }
    if (sha_handle != NULL) {
        bootloader_sha256_data(sha_handle, data_buffer, sizeof(data_buffer));
    }
    ESP_FAULT_ASSERT(memcmp(data_buffer, src, sizeof(data_buffer)) == 0);
    ESP_FAULT_ASSERT(memcmp(&metadata->secure_version, &src[1], sizeof(uint32_t)) == 0);
    return sizeof(data_buffer);
}