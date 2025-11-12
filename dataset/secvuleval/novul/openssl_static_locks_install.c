static int openssl_static_locks_install(void)
{
    int result;

    if (openssl_locks != NULL)
    {
        LogInfo("Locks already initialized");
        result = MU_FAILURE;
    }
    else
    {
        size_t malloc_size = safe_multiply_size_t(CRYPTO_num_locks(), sizeof(LOCK_HANDLE));
        if (malloc_size == SIZE_MAX ||
            (openssl_locks = malloc(malloc_size)) == NULL)
        {
            LogError("Failed to allocate locks, size:%zu", malloc_size);
            result = MU_FAILURE;
        }
        else
        {
            int i;
            for (i = 0; i < CRYPTO_num_locks(); i++)
            {
                openssl_locks[i] = Lock_Init();
                if (openssl_locks[i] == NULL)
                {
                    LogError("Failed to allocate lock %d", i);
                    break;
                }
            }

            if (i != CRYPTO_num_locks())
            {
                int j;
                for (j = 0; j < i; j++)
                {
                    Lock_Deinit(openssl_locks[j]);
                }
                result = MU_FAILURE;
            }
            else
            {
                CRYPTO_set_locking_callback(openssl_static_locks_lock_unlock_cb);

                result = 0;
            }
        }
    }
    return result;
}