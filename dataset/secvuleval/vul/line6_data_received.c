static void line6_data_received(struct urb *urb)
{
	struct usb_line6 *line6 = (struct usb_line6 *)urb->context;
	struct midi_buffer *mb = &line6->line6midi->midibuf_in;
	int done;

	if (urb->status == -ESHUTDOWN)
		return;

	if (line6->properties->capabilities & LINE6_CAP_CONTROL_MIDI) {
		done =
			line6_midibuf_write(mb, urb->transfer_buffer, urb->actual_length);

		if (done < urb->actual_length) {
			line6_midibuf_ignore(mb, done);
			dev_dbg(line6->ifcdev, "%d %d buffer overflow - message skipped\n",
				done, urb->actual_length);
		}

		for (;;) {
			done =
				line6_midibuf_read(mb, line6->buffer_message,
						   LINE6_MIDI_MESSAGE_MAXLEN,
						   LINE6_MIDIBUF_READ_RX);

			if (done <= 0)
				break;

			line6->message_length = done;
			line6_midi_receive(line6, line6->buffer_message, done);

			if (line6->process_message)
				line6->process_message(line6);
		}
	} else {
		line6->buffer_message = urb->transfer_buffer;
		line6->message_length = urb->actual_length;
		if (line6->process_message)
			line6->process_message(line6);
		line6->buffer_message = NULL;
	}

	line6_start_listen(line6);
}
