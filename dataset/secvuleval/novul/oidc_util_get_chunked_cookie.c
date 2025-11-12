char *oidc_util_get_chunked_cookie(request_rec *r, const char *cookieName, int chunkSize) {
	char *cookieValue = NULL, *chunkValue = NULL;
	int chunkCount = 0, i = 0;
	if (chunkSize == 0)
		return oidc_util_get_cookie(r, cookieName);
	chunkCount = oidc_util_get_chunked_count(r, cookieName);
	if (chunkCount == 0)
		return oidc_util_get_cookie(r, cookieName);
	if ((chunkCount < 0) || (chunkCount > 99)) {
		oidc_warn(r, "chunk count out of bounds: %d", chunkCount);
		return NULL;
	}
	for (i = 0; i < chunkCount; i++) {
		chunkValue = oidc_util_get_cookie(r, oidc_util_get_chunk_cookie_name(r, cookieName, i));
		if (chunkValue == NULL) {
			oidc_warn(r, "could not find chunk %d; aborting", i);
			break;
		}
		cookieValue = apr_psprintf(r->pool, "%s%s", cookieValue ? cookieValue : "", chunkValue);
	}
	return cookieValue;
}