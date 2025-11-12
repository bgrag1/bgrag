struct key *find_asymmetric_key(struct key *keyring,
				const struct asymmetric_key_id *id_0,
				const struct asymmetric_key_id *id_1,
				const struct asymmetric_key_id *id_2,
				bool partial)
{
	struct key *key;
	key_ref_t ref;
	const char *lookup;
	char *req, *p;
	int len;

	if (id_0) {
		lookup = id_0->data;
		len = id_0->len;
	} else if (id_1) {
		lookup = id_1->data;
		len = id_1->len;
	} else if (id_2) {
		lookup = id_2->data;
		len = id_2->len;
	} else {
		WARN_ON(1);
		return ERR_PTR(-EINVAL);
	}

	/* Construct an identifier "id:<keyid>". */
	p = req = kmalloc(2 + 1 + len * 2 + 1, GFP_KERNEL);
	if (!req)
		return ERR_PTR(-ENOMEM);

	if (!id_0 && !id_1) {
		*p++ = 'd';
		*p++ = 'n';
	} else if (partial) {
		*p++ = 'i';
		*p++ = 'd';
	} else {
		*p++ = 'e';
		*p++ = 'x';
	}
	*p++ = ':';
	p = bin2hex(p, lookup, len);
	*p = 0;

	pr_debug("Look up: \"%s\"\n", req);

	ref = keyring_search(make_key_ref(keyring, 1),
			     &key_type_asymmetric, req, true);
	if (IS_ERR(ref))
		pr_debug("Request for key '%s' err %ld\n", req, PTR_ERR(ref));
	kfree(req);

	if (IS_ERR(ref)) {
		switch (PTR_ERR(ref)) {
			/* Hide some search errors */
		case -EACCES:
		case -ENOTDIR:
		case -EAGAIN:
			return ERR_PTR(-ENOKEY);
		default:
			return ERR_CAST(ref);
		}
	}

	key = key_ref_to_ptr(ref);
	if (id_0 && id_1) {
		const struct asymmetric_key_ids *kids = asymmetric_key_ids(key);

		if (!kids->id[1]) {
			pr_debug("First ID matches, but second is missing\n");
			goto reject;
		}
		if (!asymmetric_key_id_same(id_1, kids->id[1])) {
			pr_debug("First ID matches, but second does not\n");
			goto reject;
		}
	}

	pr_devel("<==%s() = 0 [%x]\n", __func__, key_serial(key));
	return key;

reject:
	key_put(key);
	return ERR_PTR(-EKEYREJECTED);
}
