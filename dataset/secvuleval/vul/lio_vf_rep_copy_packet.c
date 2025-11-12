static void
lio_vf_rep_copy_packet(struct octeon_device *oct,
		       struct sk_buff *skb,
		       int len)
{
	if (likely(len > MIN_SKB_SIZE)) {
		struct octeon_skb_page_info *pg_info;
		unsigned char *va;

		pg_info = ((struct octeon_skb_page_info *)(skb->cb));
		if (pg_info->page) {
			va = page_address(pg_info->page) +
				pg_info->page_offset;
			memcpy(skb->data, va, MIN_SKB_SIZE);
			skb_put(skb, MIN_SKB_SIZE);
		}

		skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags,
				pg_info->page,
				pg_info->page_offset + MIN_SKB_SIZE,
				len - MIN_SKB_SIZE,
				LIO_RXBUFFER_SZ);
	} else {
		struct octeon_skb_page_info *pg_info =
			((struct octeon_skb_page_info *)(skb->cb));

		skb_copy_to_linear_data(skb, page_address(pg_info->page) +
					pg_info->page_offset, len);
		skb_put(skb, len);
		put_page(pg_info->page);
	}
}
