int cfg80211_get_station(struct net_device *dev, const u8 *mac_addr,
			 struct station_info *sinfo)
{
	struct cfg80211_registered_device *rdev;
	struct wireless_dev *wdev;

	wdev = dev->ieee80211_ptr;
	if (!wdev)
		return -EOPNOTSUPP;

	rdev = wiphy_to_rdev(wdev->wiphy);
	if (!rdev->ops->get_station)
		return -EOPNOTSUPP;

	memset(sinfo, 0, sizeof(*sinfo));

	return rdev_get_station(rdev, dev, mac_addr, sinfo);
}
