static int send_chunk(CONCRETE_IO_HANDLE tls_io, const void* buffer, size_t size, ON_SEND_COMPLETE on_send_complete, void* callback_context)
{
    int result;

    if ((tls_io == NULL) ||
        (buffer == NULL) ||
        (size == 0))
    {
        LogError("invalid argument detected: CONCRETE_IO_HANDLE tls_io = %p, const void* buffer = %p, size_t size = %lu", tls_io, buffer, (unsigned long)size);
        result = MU_FAILURE;
    }
    else
    {
        TLS_IO_INSTANCE* tls_io_instance = (TLS_IO_INSTANCE*)tls_io;
        if (tls_io_instance->tlsio_state != TLSIO_STATE_OPEN)
        {
            LogError("invalid tls_io_instance->tlsio_state: %" PRI_MU_ENUM "", MU_ENUM_VALUE(TLSIO_STATE, tls_io_instance->tlsio_state));
            result = MU_FAILURE;
        }
        else
        {
            SecPkgContext_StreamSizes  sizes;
            SECURITY_STATUS status = QueryContextAttributes(&tls_io_instance->security_context, SECPKG_ATTR_STREAM_SIZES, &sizes);
            if (status != SEC_E_OK)
            {
                LogError("QueryContextAttributes failed: %x", status);
                result = MU_FAILURE;
            }
            else
            {
                unsigned char* out_buffer;
                SecBuffer security_buffers[4];
                SecBufferDesc security_buffers_desc;
                size_t needed_buffer = safe_add_size_t(sizes.cbHeader, size);
                needed_buffer = safe_add_size_t(needed_buffer, sizes.cbTrailer);
                if (needed_buffer == SIZE_MAX ||
                    (out_buffer = (unsigned char*)malloc(needed_buffer)) == NULL)
                {
                    LogError("malloc failed, size:%zu", needed_buffer);
                    result = MU_FAILURE;
                }
                else
                {
                    (void)memcpy(out_buffer + sizes.cbHeader, buffer, size);

                    security_buffers[0].BufferType = SECBUFFER_STREAM_HEADER;
                    security_buffers[0].cbBuffer = sizes.cbHeader;
                    security_buffers[0].pvBuffer = out_buffer;
                    security_buffers[1].BufferType = SECBUFFER_DATA;
                    security_buffers[1].cbBuffer = (unsigned long)size;
                    security_buffers[1].pvBuffer = out_buffer + sizes.cbHeader;
                    security_buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
                    security_buffers[2].cbBuffer = sizes.cbTrailer;
                    security_buffers[2].pvBuffer = out_buffer + sizes.cbHeader + size;
                    security_buffers[3].cbBuffer = 0;
                    security_buffers[3].BufferType = SECBUFFER_EMPTY;
                    security_buffers[3].pvBuffer = 0;

                    security_buffers_desc.cBuffers = sizeof(security_buffers) / sizeof(security_buffers[0]);
                    security_buffers_desc.pBuffers = security_buffers;
                    security_buffers_desc.ulVersion = SECBUFFER_VERSION;

                    status = EncryptMessage(&tls_io_instance->security_context, 0, &security_buffers_desc, 0);
                    if (FAILED(status))
                    {
                        LogError("EncryptMessage failed: %x", status);
                        result = MU_FAILURE;
                    }
                    else
                    {
                        if (xio_send(tls_io_instance->socket_io, out_buffer, (size_t)security_buffers[0].cbBuffer + (size_t)security_buffers[1].cbBuffer + (size_t)security_buffers[2].cbBuffer, on_send_complete, callback_context) != 0)
                        {
                            LogError("xio_send failed");
                            result = MU_FAILURE;
                        }
                        else
                        {
                            result = 0;
                        }
                    }

                    free(out_buffer);
                }
            }
        }
    }

    return result;
}