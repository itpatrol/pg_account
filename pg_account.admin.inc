<?php

function pg_account_history(){
  drupal_add_css(drupal_get_path('module', 'pg_account') .'/css/pg_account.css');

//  $filters = formfilterapi_build_filter_query('pg_account_operator_history');
  $header = array(
    'date' => array('data' => t('Date'), 'field' => 'pgt.changed', 'sort' => 'desc'),
    'amount' => array('data' => t('Amount'), 'field' => 'pgt.amount'),
    'user' => array('data' => t('User'), 'field' => 'u.name'),
    'description' => array('data' => t('Description')),
    'status' => array('data' => t('Payment status'), 'field' => 'pgt.status'),
    'workflow' => array('data' => t('Operation status'), 'field' => 'pgt.workflow')
  );

  $query = db_select('pg_account_transaction', 'pat')->extend('AltPager')->extend('TableSort');
  $query->fields('pat');    
  $query->leftjoin('pgapi_transaction', 'pgt', 'pat.pgapi_txnid = %alias.txnid');
  $query->fields('pgt');    
  $query->extend('TableSort');
  $query->orderByHeader($header);
  
  $transactions= $query->execute()->fetchAll();

  
  foreach ($transactions as $txn) {

    $description = $txn->description ? $txn->description  : $txn->title;

    if($txn->status == PG_CANCELED){
        $description .= $txn->title.'<br>'.$txn->description;
    }    
    $account = user_load($txn->uid);
    $class = array($txn->operation, 'status-'.$txn->status, 'workflow-'.$txn->workflow );    
    $rows[] = array(
      'data'=>array(
        array( 'data' => format_date($txn->changed,'custom','M d, Y'), 'class' => 'date'),
        array( 'data' => pgapi_format_price($txn->amount), 'class' => array('amount')),
        theme('username',array('account' => $account)),
        $description,
        pgapi_get_status($txn->status),
        pgapi_get_workflow($txn->workflow),
      ),
      'class' => $class,
    );

  }//while
  if(empty($rows)){
    $rows[] = array(array('data' => t('No data available.'), 'colspan' => '6'));
  }
  $pagerCountShow = theme('altpager');
  
  $output = $pagerCountShow;
  $output .= '<div width=99% style="overflow-x: auto; ">';
  $output .=  theme('table', array( 'header' => $header, 'rows' => $rows));
  $output .= "</div>";
  $output .= $pagerCountShow;
  
  
  return $output;

}

/* Transaction status callbacks*/

function pg_account_fail ($t){
  global $user;
  if($user->uid!=1 && $user->uid != $t->uid){
    drupal_access_denied();
  }

  return theme('pg_account_fail',$t);
}


function pg_account_complete ($t){
  global $user;
  if($user->uid != 1 and $user->uid != $t->uid){
   drupal_access_denied();
  }
  
  return theme('pg_account_complete',$t);
}

function pg_account_pending($t){
  global $user;
  if($user->uid != 1 and $user->uid != $t->uid){
   drupal_access_denied();
  }
  
  return theme('pg_account_pending',$t);
}

function pg_account_settings(){
  
  $roles = array_map('check_plain', user_roles(TRUE));

  $form['pg_account_operator_messages'] = array(
    '#type' => 'fieldset',
    '#title' => t('Operator messages'),
    '#description' => t('Please use tokens to form title and message for notifications based on balance adjustment by operator.')
  );

  $form['pg_account_operator_messages']['add'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add balance'),
  );
  $form['pg_account_operator_messages']['add']['pg_account_admin_title_add'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => variable_get('pg_account_admin_title_add',PG_ACCOUNT_ADD_BALANCE_ADMIN_TITLE),
    '#size' => 150,    
    '#maxlength' => 255,
  );

  $form['pg_account_operator_messages']['add']['pg_account_admin_message_add'] = array(
    '#type' => 'text_format',
    '#title' => t('Message'),
    '#default_value' => variable_get('pg_account_admin_message_add_body',PG_ACCOUNT_ADD_BALANCE_ADMIN_MESSAGE),
    '#rows' => 6,
    '#resizable' => TRUE,    
    '#format' => variable_get('pg_account_admin_message_add_format',NULL),
  );

  $form['pg_account_operator_messages']['add']['tokens'] = array(
    '#theme' => 'token_tree',
    '#token_types' => array('pg_account'),
  );

  $form['pg_account_operator_messages']['sub'] = array(
    '#type' => 'fieldset',
    '#title' => t('Withdraw balance'),
  );
  $form['pg_account_operator_messages']['sub']['pg_account_admin_title_sub'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => variable_get('pg_account_admin_title_sub',PG_ACCOUNT_SUB_BALANCE_ADMIN_TITLE),
    '#size' => 150,    
    '#maxlength' => 255,
  );

  $form['pg_account_operator_messages']['sub']['pg_account_admin_message_sub'] = array(
    '#type' => 'text_format',
    '#title' => t('Message'),
    '#default_value' => variable_get('pg_account_admin_message_sub_body',PG_ACCOUNT_SUB_BALANCE_ADMIN_MESSAGE),
    '#rows' => 6,
    '#resizable' => TRUE,    
    '#format' => variable_get('pg_account_admin_message_sub_format',NULL),
  );

  $form['pg_account_operator_messages']['sub']['tokens'] = array(
    '#theme' => 'token_tree',
    '#token_types' => array('pg_account'),
  );
  
  $form['pg_account_user_messages'] = array(
    '#type' => 'fieldset',
    '#title' => t('User messages'),
    '#description' => t('Please use tokens to form title and message for notifications based on balance adjustment by operator .')
  );

  $form['pg_account_user_messages']['add'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add balance'),
  );

  $form['pg_account_user_messages']['add']['pg_account_user_title_add'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => variable_get('pg_account_user_title_add',PG_ACCOUNT_ADD_BALANCE_USER_TITLE),
    '#size' => 150,    
    '#maxlength' => 255,
  );

  $form['pg_account_user_messages']['add']['pg_account_user_message_add'] = array(
    '#type' => 'text_format',
    '#title' => t('Message'),
    '#default_value' => variable_get('pg_account_user_message_add_body',PG_ACCOUNT_ADD_BALANCE_USER_MESSAGE),
    '#rows'               => 6,
    '#resizable'          => TRUE,    
    '#format' => variable_get('pg_account_user_message_add_format',NULL),
  );

  $form['pg_account_user_messages']['add']['tokens'] = array(
    '#theme' => 'token_tree',
    '#token_types' => array('pg_account'),
  );

  $form['pg_account_user_messages']['sub'] = array(
    '#type' => 'fieldset',
    '#title' => t('Withdraw from balance'),
  );

  $form['pg_account_user_messages']['sub']['pg_account_user_title_sub'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => variable_get('pg_account_user_title_sub',PG_ACCOUNT_SUB_BALANCE_USER_TITLE),
    '#size' => 150,    
    '#maxlength' => 255,
  );

  $form['pg_account_user_messages']['sub']['pg_account_user_message_sub'] = array(
    '#type' => 'text_format',
    '#title' => t('Message'),
    '#default_value' => variable_get('pg_account_user_message_sub_body',PG_ACCOUNT_SUB_BALANCE_USER_MESSAGE),
    '#rows'               => 6,
    '#resizable'          => TRUE,    
    '#format' => variable_get('pg_account_user_message_sub_format',NULL),
  );

  $form['pg_account_user_messages']['sub']['tokens'] = array(
    '#theme' => 'token_tree',
    '#token_types' => array('pg_account'),
  );  

  $form['pg_account_user_messages']['send'] = array(
    '#type' => 'fieldset',
    '#title' => t('Send transfer'),
  );

  $form['pg_account_user_messages']['send']['pg_account_user_title_add'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => variable_get('pg_account_user_title_send',PG_ACCOUNT_SEND_BALANCE_USER_TITLE),
    '#size' => 150,    
    '#maxlength' => 255,
  );

  $form['pg_account_user_messages']['send']['pg_account_user_message_send'] = array(
    '#type' => 'text_format',
    '#title' => t('Message'),
    '#default_value' => variable_get('pg_account_user_message_send_body',PG_ACCOUNT_SEND_BALANCE_USER_MESSAGE),
    '#rows'               => 6,
    '#resizable'          => TRUE,    
    '#format' => variable_get('pg_account_user_message_send_format',NULL),
  );

  $form['pg_account_user_messages']['send']['tokens'] = array(
    '#theme' => 'token_tree',
    '#token_types' => array('pg_account'),
  );

  $form['pg_account_user_messages']['receive'] = array(
    '#type' => 'fieldset',
    '#title' => t('Receive transfer'),
  );

  $form['pg_account_user_messages']['receive']['pg_account_user_title_receive'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => variable_get('pg_account_user_title_receive',PG_ACCOUNT_RECEIVE_BALANCE_USER_TITLE),
    '#size' => 150,    
    '#maxlength' => 255,
  );

  $form['pg_account_user_messages']['receive']['pg_account_user_message_receive'] = array(
    '#type' => 'text_format',
    '#title' => t('Message'),
    '#default_value' => variable_get('pg_account_user_message_receive_body',PG_ACCOUNT_RECEIVE_BALANCE_USER_MESSAGE),
    '#rows'               => 6,
    '#resizable'          => TRUE,    
    '#format' => variable_get('pg_account_user_message_receive_format',NULL),
  );

  $form['pg_account_user_messages']['receive']['tokens'] = array(
    '#theme' => 'token_tree',
    '#token_types' => array('pg_account'),
  );  
  
  $form['pg_account_roles'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Roles'),
    '#default_value' => variable_get('pg_account_roles',array()),
    '#options' => $roles,
    '#description' => t('Please select role to send notification about user balance changes.')
  );
  
  $form['submit'] = array(
    '#type'  => 'submit',
    '#value' => t('Save changes'),
  );

  return $form;
}


function pg_account_settings_submit($form, &$form_state){

  variable_set('pg_account_roles',$form_state['values']['pg_account_roles']);

  variable_set('pg_account_admin_title_add',$form_state['values']['pg_account_admin_title_add']);

  variable_set('pg_account_admin_message_add_body',$form_state['values']['pg_account_admin_message_add']['value']);    

  variable_set('pg_account_admin_message_add_format',$form_state['values']['pg_account_admin_message_add']['format']);    

  variable_set('pg_account_admin_title_sub',$form_state['values']['pg_account_admin_title_sub']);

  variable_set('pg_account_admin_message_sub_body',$form_state['values']['pg_account_admin_message_sub']['value']);    

  variable_set('pg_account_admin_message_sub_format',$form_state['values']['pg_account_admin_message_sub']['format']);    

  variable_set('pg_account_user_title_add',$form_state['values']['pg_account_user_title_add']);

  variable_set('pg_account_user_message_add_body',$form_state['values']['pg_account_user_message_add']['value']);    

  variable_set('pg_account_user_message_add_format',$form_state['values']['pg_account_user_message_add']['format']);    

  variable_set('pg_account_user_title_sub',$form_state['values']['pg_account_user_title_sub']);

  variable_set('pg_account_user_message_sub_body',$form_state['values']['pg_account_user_message_sub']['value']);    

  variable_set('pg_account_user_message_sub_format',$form_state['values']['pg_account_user_message_sub']['format']);    

  variable_set('pg_account_user_title_send',$form_state['values']['pg_account_user_title_send']);

  variable_set('pg_account_user_message_send_body',$form_state['values']['pg_account_user_message_send']['value']);

  variable_set('pg_account_user_message_send_format',$form_state['values']['pg_account_user_message_send']['format']);    

  variable_set('pg_account_user_title_receive',$form_state['values']['pg_account_user_title_receive']);

  variable_set('pg_account_user_message_receive_body',$form_state['values']['pg_account_user_message_receive']['value']);    

  variable_set('pg_account_user_message_receive_format',$form_state['values']['pg_account_user_message_receive']['format']);    

  drupal_set_message(t('Changes saved.'));      
}
