static void
iwl_mvm_ftm_set_secured_ranging(struct iwl_mvm *mvm, struct ieee80211_vif *vif,
				u8 *bssid, u8 *cipher, u8 *hltk, u8 *tk,
				u8 *rx_pn, u8 *tx_pn, __le32 *flags)
{
	struct iwl_mvm_ftm_pasn_entry *entry;
// #ifdef CONFIG_IWLWIFI_DEBUGFS
	struct iwl_mvm_vif *mvmvif = iwl_mvm_vif_from_mac80211(vif);

	if (mvmvif->ftm_unprotected)
		return;
#endif

	if (!(le32_to_cpu(*flags) & (IWL_INITIATOR_AP_FLAGS_NON_TB |
		       IWL_INITIATOR_AP_FLAGS_TB)))
		return;

	lockdep_assert_held(&mvm->mutex);

	list_for_each_entry(entry, &mvm->ftm_initiator.pasn_list, list) {
		if (memcmp(entry->addr, bssid, sizeof(entry->addr)))
			continue;

		*cipher = entry->cipher;

		if (entry->flags & IWL_MVM_PASN_FLAG_HAS_HLTK)
			memcpy(hltk, entry->hltk, sizeof(entry->hltk));
		else
			memset(hltk, 0, sizeof(entry->hltk));

		if (vif->cfg.assoc &&
		    !memcmp(vif->bss_conf.bssid, bssid, ETH_ALEN)) {
			struct iwl_mvm_ftm_iter_data target;

			target.bssid = bssid;
			target.cipher = cipher;
			ieee80211_iter_keys(mvm->hw, vif, iter, &target);
		} else {
			memcpy(tk, entry->tk, sizeof(entry->tk));
		}

		memcpy(rx_pn, entry->rx_pn, sizeof(entry->rx_pn));
		memcpy(tx_pn, entry->tx_pn, sizeof(entry->tx_pn));

		FTM_SET_FLAG(SECURED);
		return;
	}
}
