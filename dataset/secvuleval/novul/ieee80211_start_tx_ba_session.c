int ieee80211_start_tx_ba_session(struct ieee80211_sta *pubsta, u16 tid,
				  u16 timeout)
{
	struct sta_info *sta = container_of(pubsta, struct sta_info, sta);
	struct ieee80211_sub_if_data *sdata = sta->sdata;
	struct ieee80211_local *local = sdata->local;
	struct tid_ampdu_tx *tid_tx;
	int ret = 0;

	trace_api_start_tx_ba_session(pubsta, tid);

	if (WARN(sta->reserved_tid == tid,
		 "Requested to start BA session on reserved tid=%d", tid))
		return -EINVAL;

	if (!pubsta->deflink.ht_cap.ht_supported &&
	    !pubsta->deflink.vht_cap.vht_supported &&
	    !pubsta->deflink.he_cap.has_he &&
	    !pubsta->deflink.eht_cap.has_eht)
		return -EINVAL;

	if (WARN_ON_ONCE(!local->ops->ampdu_action))
		return -EINVAL;

	if ((tid >= IEEE80211_NUM_TIDS) ||
	    !ieee80211_hw_check(&local->hw, AMPDU_AGGREGATION) ||
	    ieee80211_hw_check(&local->hw, TX_AMPDU_SETUP_IN_HW))
		return -EINVAL;

	if (WARN_ON(tid >= IEEE80211_FIRST_TSPEC_TSID))
		return -EINVAL;

	ht_dbg(sdata, "Open BA session requested for %pM tid %u\n",
	       pubsta->addr, tid);

	if (sdata->vif.type != NL80211_IFTYPE_STATION &&
	    sdata->vif.type != NL80211_IFTYPE_MESH_POINT &&
	    sdata->vif.type != NL80211_IFTYPE_AP_VLAN &&
	    sdata->vif.type != NL80211_IFTYPE_AP &&
	    sdata->vif.type != NL80211_IFTYPE_ADHOC)
		return -EINVAL;

	if (test_sta_flag(sta, WLAN_STA_BLOCK_BA)) {
		ht_dbg(sdata,
		       "BA sessions blocked - Denying BA session request %pM tid %d\n",
		       sta->sta.addr, tid);
		return -EINVAL;
	}

	if (test_sta_flag(sta, WLAN_STA_MFP) &&
	    !test_sta_flag(sta, WLAN_STA_AUTHORIZED)) {
		ht_dbg(sdata,
		       "MFP STA not authorized - deny BA session request %pM tid %d\n",
		       sta->sta.addr, tid);
		return -EINVAL;
	}

	/*
	 * 802.11n-2009 11.5.1.1: If the initiating STA is an HT STA, is a
	 * member of an IBSS, and has no other existing Block Ack agreement
	 * with the recipient STA, then the initiating STA shall transmit a
	 * Probe Request frame to the recipient STA and shall not transmit an
	 * ADDBA Request frame unless it receives a Probe Response frame
	 * from the recipient within dot11ADDBAFailureTimeout.
	 *
	 * The probe request mechanism for ADDBA is currently not implemented,
	 * but we only build up Block Ack session with HT STAs. This information
	 * is set when we receive a bss info from a probe response or a beacon.
	 */
	if (sta->sdata->vif.type == NL80211_IFTYPE_ADHOC &&
	    !sta->sta.deflink.ht_cap.ht_supported) {
		ht_dbg(sdata,
		       "BA request denied - IBSS STA %pM does not advertise HT support\n",
		       pubsta->addr);
		return -EINVAL;
	}

	spin_lock_bh(&sta->lock);

	/* we have tried too many times, receiver does not want A-MPDU */
	if (sta->ampdu_mlme.addba_req_num[tid] > HT_AGG_MAX_RETRIES) {
		ret = -EBUSY;
		goto err_unlock_sta;
	}

	/*
	 * if we have tried more than HT_AGG_BURST_RETRIES times we
	 * will spread our requests in time to avoid stalling connection
	 * for too long
	 */
	if (sta->ampdu_mlme.addba_req_num[tid] > HT_AGG_BURST_RETRIES &&
	    time_before(jiffies, sta->ampdu_mlme.last_addba_req_time[tid] +
			HT_AGG_RETRIES_PERIOD)) {
		ht_dbg(sdata,
		       "BA request denied - %d failed requests on %pM tid %u\n",
		       sta->ampdu_mlme.addba_req_num[tid], sta->sta.addr, tid);
		ret = -EBUSY;
		goto err_unlock_sta;
	}

	tid_tx = rcu_dereference_protected_tid_tx(sta, tid);
	/* check if the TID is not in aggregation flow already */
	if (tid_tx || sta->ampdu_mlme.tid_start_tx[tid]) {
		ht_dbg(sdata,
		       "BA request denied - session is not idle on %pM tid %u\n",
		       sta->sta.addr, tid);
		ret = -EAGAIN;
		goto err_unlock_sta;
	}

	/* prepare A-MPDU MLME for Tx aggregation */
	tid_tx = kzalloc(sizeof(struct tid_ampdu_tx), GFP_ATOMIC);
	if (!tid_tx) {
		ret = -ENOMEM;
		goto err_unlock_sta;
	}

	skb_queue_head_init(&tid_tx->pending);
	__set_bit(HT_AGG_STATE_WANT_START, &tid_tx->state);

	tid_tx->timeout = timeout;
	tid_tx->sta = sta;
	tid_tx->tid = tid;

	/* response timer */
	timer_setup(&tid_tx->addba_resp_timer, sta_addba_resp_timer_expired, 0);

	/* tx timer */
	timer_setup(&tid_tx->session_timer,
		    sta_tx_agg_session_timer_expired, TIMER_DEFERRABLE);

	/* assign a dialog token */
	sta->ampdu_mlme.dialog_token_allocator++;
	tid_tx->dialog_token = sta->ampdu_mlme.dialog_token_allocator;

	/*
	 * Finally, assign it to the start array; the work item will
	 * collect it and move it to the normal array.
	 */
	sta->ampdu_mlme.tid_start_tx[tid] = tid_tx;

	wiphy_work_queue(local->hw.wiphy, &sta->ampdu_mlme.work);

	/* this flow continues off the work */
 err_unlock_sta:
	spin_unlock_bh(&sta->lock);
	return ret;
}
