int password_equal(const char *user_input, const char *secret) {
    size_t i = 0;
    size_t j = 0;
    char out = 0;

    while (1) {
        /* Out stays zero if the strings are the same. */
        out |= user_input[i] ^ secret[j];

        /* Stop at end of user_input. */
        if (user_input[i] == 0) break;
        i++;

        /* Don't go past end of secret. */
        if (secret[j] != 0) j++;
    }

    /* Check length after loop, otherwise early exit would leak length. */
    out |= (i != j); /* Secret was shorter. */
    out |= (secret[j] != 0); /* Secret was longer; j is not the end. */
    return out == 0;
}