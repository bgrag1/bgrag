static int __spi_sync(struct spi_device *spi, struct spi_message *message)
{
	DECLARE_COMPLETION_ONSTACK(done);
	unsigned long flags;
	int status;
	struct spi_controller *ctlr = spi->controller;

	if (__spi_check_suspended(ctlr)) {
		dev_warn_once(&spi->dev, "Attempted to sync while suspend\n");
		return -ESHUTDOWN;
	}

	status = spi_maybe_optimize_message(spi, message);
	if (status)
		return status;

	SPI_STATISTICS_INCREMENT_FIELD(ctlr->pcpu_statistics, spi_sync);
	SPI_STATISTICS_INCREMENT_FIELD(spi->pcpu_statistics, spi_sync);

	/*
	 * Checking queue_empty here only guarantees async/sync message
	 * ordering when coming from the same context. It does not need to
	 * guard against reentrancy from a different context. The io_mutex
	 * will catch those cases.
	 */
	if (READ_ONCE(ctlr->queue_empty) && !ctlr->must_async) {
		message->actual_length = 0;
		message->status = -EINPROGRESS;

		trace_spi_message_submit(message);

		SPI_STATISTICS_INCREMENT_FIELD(ctlr->pcpu_statistics, spi_sync_immediate);
		SPI_STATISTICS_INCREMENT_FIELD(spi->pcpu_statistics, spi_sync_immediate);

		__spi_transfer_message_noqueue(ctlr, message);

		return message->status;
	}

	/*
	 * There are messages in the async queue that could have originated
	 * from the same context, so we need to preserve ordering.
	 * Therefor we send the message to the async queue and wait until they
	 * are completed.
	 */
	message->complete = spi_complete;
	message->context = &done;

	spin_lock_irqsave(&ctlr->bus_lock_spinlock, flags);
	status = __spi_async(spi, message);
	spin_unlock_irqrestore(&ctlr->bus_lock_spinlock, flags);

	if (status == 0) {
		wait_for_completion(&done);
		status = message->status;
	}
	message->complete = NULL;
	message->context = NULL;

	return status;
}
