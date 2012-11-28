<?php
/**
 * Contains batch processing functions to send many emails at once.
 */

/**
 * Set up the batch to run.
 *
 * @param array $uids
 *   Array of uids to try.
 */
function maya_pass_reset_batch_send_mails_build($uids) {
  $total_mails = count($uids);
  $operations = array();
  foreach ($uids as $uid) {
    $operations[] = array('maya_pass_reset_batch_send_mail', array($uid, $total_mails));
  }

  $batch = array(
    'title' => t('Sending emails'),
    'finished' => 'maya_pass_reset_batch_send_mails_finished',
    'operations' => $operations,
    'progress_message' => t('Processed @current out of @total.'),
    'file' => drupal_get_path('module', 'maya_pass_reset') . '/maya_pass_reset.batch.inc',
  );

  drupal_set_message(t('Sending @count reset mails.', array('@count' => $total_mails)));

  return $batch;
}

/**
 * Send a single mail.
 *
 * @param int $uid
 *   The user to send an email to.
 * @param int $total
 *   Total number of emails to send for whole batch.
 * @param array $context
 *   Current state of the batch.
 */
function maya_pass_reset_batch_send_mail($uid, $total, &$context) {
  // First run through.
  if (empty($context['sandbox'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['current_uid'] = 0;
    $context['sandbox']['max'] = $total;
  }
  $context['sandbox']['current_uid'] = $uid;
  $account = user_load($uid);
  maya_pass_reset_send_mail($account);
  $context['sandbox']['progress']++;
  $context['results'][] = $account->mail;
  $context['message'] = t('Sent reset mail to @email', array('@email' => $account->mail));
  $context['finished'] = 1;

}

/**
 * Sends a user a password reset mail.
 *
 * @param object $account
 *   The user account to notify.
 */
function maya_pass_reset_send_mail($account) {
  if (
    variable_get('maya_pass_reset_send_mails', FALSE) &&
    _maya_pass_reset_sent($account->uid) == FALSE
  ) {
    // Send the user a password reset mail.
    _user_mail_notify('password_reset', $account);
    _maya_pass_reset_send_mail_log($account);
    maya_pass_reset_sleep();
  }
  else {
    $message = t(
      'Not sending mail to @mail. Confirm you want to send mails in sending options, or check that this user (@uid) has not already been sent a mail.',
      array('@mail' => $account->mail, '@uid' => $account->uid)
    );
    drupal_set_message($message);
  }
}

/**
 * Logs a mail as sent to a user.
 *
 * @param object $account
 *   User acocunt object.
 */
function _maya_pass_reset_send_mail_log($account) {
  db_insert('maya_pass_reset')
    ->fields(
      array(
        'uid' => $account->uid,
        'mail' => $account->mail,
        // Use time() instead of REQUEST_TIME, as we're in a batch.
        'sent' => time(),
      )
    )
    ->execute();
}

function maya_pass_reset_batch_send_mails_finished($success, $results, $operations) {
  if ($success) {
    $message = format_plural(count($results), 'Sent one password reset email.', 'Sent @count password reset emails.');
  }
  else {
    $message = t('Something went wrong :(');
  }

  drupal_set_message($message);
}

/**
 * Sleeps the script for a number of milliseconds. If the number is not defined,
 * the variable maya_pass_sleep_time() is used.
 *
 * @param int $milliseconds (optional)
 *   Number of millseconds to sleep for.
 *
 * @see maya_pass_reset_config_form()
 */
function maya_pass_reset_sleep($milliseconds = NULL) {
  // If no value is passed, use the value set in the pass reset config form.
  if ($milliseconds == NULL) {
    $milliseconds = variable_get('maya_pass_sleep_time', 0);
  }
  if ($milliseconds) {
    //dd('Sleeping for '. $milliseconds . ' milliseconds');
    $microseconds = $milliseconds * 1000;
    usleep($microseconds);
  }
}

/**
 * Checks whether an email has been sent for a given uid.
 *
 * @param int $uid
 *   Uid to check.
 *
 * @return bool
 *   TRUE if this user has been sent an email, FALSE if not.
 */
function _maya_pass_reset_sent($uid) {
  $result = db_query("SELECT count(uid) as sent FROM {maya_pass_reset} WHERE uid = :uid", array(':uid' => $uid));
  return (bool) $result->fetchColumn();
}
