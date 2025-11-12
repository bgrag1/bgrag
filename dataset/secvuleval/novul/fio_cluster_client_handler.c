static void fio_cluster_client_handler(struct cluster_pr_s *pr) {
  /* what to do? */
  switch ((fio_cluster_message_type_e)pr->type) {
  case FIO_CLUSTER_MSG_FORWARD: /* fallthrough */
  case FIO_CLUSTER_MSG_JSON:
    fio_publish2process(fio_msg_internal_dup(pr->msg));
    break;
  case FIO_CLUSTER_MSG_SHUTDOWN:
    fio_stop();
    kill(getpid(), SIGINT);
  case FIO_CLUSTER_MSG_ERROR:         /* fallthrough */
  case FIO_CLUSTER_MSG_PING:          /* fallthrough */
  case FIO_CLUSTER_MSG_ROOT:          /* fallthrough */
  case FIO_CLUSTER_MSG_ROOT_JSON:     /* fallthrough */
  case FIO_CLUSTER_MSG_PUBSUB_SUB:    /* fallthrough */
  case FIO_CLUSTER_MSG_PUBSUB_UNSUB:  /* fallthrough */
  case FIO_CLUSTER_MSG_PATTERN_SUB:   /* fallthrough */
  case FIO_CLUSTER_MSG_PATTERN_UNSUB: /* fallthrough */

  default:
    break;
  }
}