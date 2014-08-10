<?php

/**
 * Implements hook_token_info().
 */
function pg_account_token_info() {
  $info['types']['pg_account'] = array(
    'name' => t('PG Account'),
    'description' => t('Tokens related to PG Account module.'),
    'needs-data' => 'pg_account',
  );

  // Tokens
  $info['tokens']['pg_account']['operatorname'] = array(
    'name' => t('Operator username'),
    'description' => t("The username of operator who changed balance."),
  );

  $info['tokens']['pg_account']['username'] = array(
    'name' => t('Username'),
    'description' => t("The username of the user who has changed balance or who sent transfer."),
  );

  $info['tokens']['pg_account']['username_received'] = array(
    'name' => t('Username who received transfer'),
    'description' => t("The username of the user who received transfer balance."),
  );

  $info['tokens']['pg_account']['balance'] = array(
    'name' => t('Balance'),
    'description' => t("The new, updated balance of username."),
  );

  $info['tokens']['pg_account']['balance-url'] = array(
    'name' => t('Balance url'),
    'description' => t("Provide link to balance page."),
  );

  $info['tokens']['pg_account']['amount'] = array(
    'name' => t('Amount'),
    'description' => t("Transaction amount."),
  );

  $info['tokens']['pg_account']['description'] = array(
    'name' => t('Description'),
    'description' => t("Description provided by user."),
  );

  return $info;
}

/**
 * Implements hook_tokens().
 */
function pg_account_tokens($type, $tokens, array $data = array(), array $options = array()) {
  $replacements = array();
  $sanitize = !empty($options['sanitize']);
  if(isset($data['pg_account'])){
    $replace = (object)$data['pg_account'];
    if($type == 'pg_account'){
      foreach ($tokens as $name => $original) {
        switch ($name) {
          case 'balance':
              $replacements[$original] = $replace->balance;
            break;
          case 'balance-url':
              if(isset($replace->uid)){
                $replacements[$original] = l(t('Balance'), 'user/' . $replace->uid . '/balance');
              }
              else {
                $account = user_load_by_name($replace->username);
                $replacements[$original] = l(t('Balance'), 'user/' . $account->uid . '/balance');
              }
            break;
          case 'amount':
              $replacements[$original] = $replace->amount;
            break;
          case 'username':
              $replacements[$original] = $sanitize ? check_plain($replace->username) : $replace->username;
            break;
          case 'username_received':
              $replacements[$original] = $sanitize ? check_plain($replace->username_received) : $replace->username_received;
            break;
          case 'operatorname':
              $replacements[$original] = $sanitize ? check_plain($replace->operatorname) : $replace->operatorname;
            break;
          case 'description':
              $replacements[$original] = $replace->description;
            break;
        }
      }
    }
  }
  return $replacements;  
}