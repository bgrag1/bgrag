static void
pk_transaction_finished_emit (PkTransaction *transaction,
			      PkExitEnum exit_enum,
			      guint time_ms)
{
	g_debug ("emitting finished '%s', %i",
		 pk_exit_enum_to_string (exit_enum),
		 time_ms);
	g_dbus_connection_emit_signal (transaction->priv->connection,
				       NULL,
				       transaction->priv->tid,
				       PK_DBUS_INTERFACE_TRANSACTION,
				       "Finished",
				       g_variant_new ("(uu)",
						      exit_enum,
						      time_ms),
				       NULL);

	/* For the transaction list */
	g_signal_emit (transaction, signals[SIGNAL_FINISHED], 0);
}