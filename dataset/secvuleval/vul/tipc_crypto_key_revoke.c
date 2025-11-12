static int tipc_crypto_key_revoke(struct net *net, u8 tx_key)
{
	struct tipc_crypto *tx = tipc_net(net)->crypto_tx;
	struct tipc_key key;

	spin_lock(&tx->lock);
	key = tx->key;
	WARN_ON(!key.active || tx_key != key.active);

	/* Free the active key */
	tipc_crypto_key_set_state(tx, key.passive, 0, key.pending);
	tipc_crypto_key_detach(tx->aead[key.active], &tx->lock);
	spin_unlock(&tx->lock);

	pr_warn("%s: key is revoked\n", tx->name);
	return -EKEYREVOKED;
}