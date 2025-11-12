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
        openssl_locks = malloc(CRYPTO_num_locks() * sizeof(LOCK_HANDLE));
        if (openssl_locks == NULL)
        {
            LogError("Failed to allocate locks");
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