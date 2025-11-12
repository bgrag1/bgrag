static void cm_work_handler(struct work_struct *_work)
{
	struct iwcm_work *work = container_of(_work, struct iwcm_work, work);
	struct iw_cm_event levent;
	struct iwcm_id_private *cm_id_priv = work->cm_id;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&cm_id_priv->lock, flags);
	while (!list_empty(&cm_id_priv->work_list)) {
		work = list_first_entry(&cm_id_priv->work_list,
					struct iwcm_work, list);
		list_del_init(&work->list);
		levent = work->event;
		put_work(work);
		spin_unlock_irqrestore(&cm_id_priv->lock, flags);

		if (!test_bit(IWCM_F_DROP_EVENTS, &cm_id_priv->flags)) {
			ret = process_event(cm_id_priv, &levent);
			if (ret)
				destroy_cm_id(&cm_id_priv->id);
		} else
			pr_debug("dropping event %d\n", levent.event);
		if (iwcm_deref_id(cm_id_priv))
			return;
		spin_lock_irqsave(&cm_id_priv->lock, flags);
	}
	spin_unlock_irqrestore(&cm_id_priv->lock, flags);
}
