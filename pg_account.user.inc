<?php 

function pg_account_account($account){

  $balance=pg_account_balance_load($account->uid);
  
  $data=array(
    'user' => $account,
    'balance' => pgapi_format_price('pg_account',$balance->balance,pg_account_getCurrentSymbol($account)),
    'tabs' => $tabs,
    'transactions' => pg_account_personal_history($account),
  );
  
  return theme('pg_account_account',$data);
}


function pg_account_personal_history($account){
  drupal_add_css(drupal_get_path('module', 'pg_account') .'/css/pg_account.css');
  if(empty($account)){
    global $user;
    $account = $user;
  }
  
  $header = array(
    'date' => array('data' => t('Date'), 'field' => 'pgt.changed', 'sort' => 'desc'),
    'amount' => array('data' => t('Amount'), 'field' => 'pgt.amount'),
    'description' => array('data' => t('Description')),
    'status' => array('data' => t('Payment status'), 'field' => 'pgt.status')
  );

  $query = db_select('pg_account_transaction', 'pat')->extend('AltPager')->extend('TableSort')->extend('FormFilterAPI');
  $query->fields('pat');    
  $query->leftjoin('pgapi_transaction', 'pgt', 'pat.pgapi_txnid = %alias.txnid');
  $query->fields('pgt', array('status','workflow', 'description', 'title', 'changed'));    
  $query->condition('pat.uid', $account->uid);
  $query->processFilters('pg_account_personal_history');
  $query->extend('TableSort');
  
  $query->orderByHeader($header);
  
  $transactions= $query->execute()->fetchAll();

  foreach ($transactions as $txn) {
    $description = $txn->title.'<br>' ; //.$txn->description;
    if($txn->status == PG_COMPLETED){
      if(!empty($txn->gateway)){
        if($txn->amount > 0){
          $description .= t('Paid by: !gateway',array('!gateway' => module_invoke($txn->gateway, 'pgapi_gw','display name'))) . '<br>';
        }else{
        }
      }
    }
    if(!empty($txn->description)){
      $description .= t('Description:') . ' ' . $txn->description;
    }
    
    $class = array($txn->operation, 'status-'.$txn->status, 'workflow-'.$txn->workflow );    
    
    $rows[] = array(
      'data' => array(
        array( 'data' => format_date($txn->changed,'custom','M d, Y'), 'class' => array('date')),
        array( 'data' => pgapi_format_price($txn->amount), 'class' => array('amount')),
        $description,
        pgapi_get_status($txn->status)
      ),
      'class' => $class,
    );
  }//while
  if(empty($rows)){
    $rows[] = array(array('data' => t('No data available.'), 'colspan' => '5'));
  }
  $pagerCountShow = theme('altpager');
  
  $balance = pg_account_balance_load($account->uid);
  $output = theme('user_balance', array( 'balance' => $balance));
  
  $form = formfilterapi_get_form('pg_account_personal_history');
  $output .= drupal_render($form);
  $output .= '<br>';  
  $output .= $pagerCountShow;
  $output .= '<div width=99% style="overflow-x: auto; ">';
  $output .=  theme('table', array( 'header' => $header, 'rows' => $rows));
  $output .= "</div>";
  $output .= $pagerCountShow;
  return $output;

}

function pg_account_account_add($account){
  global $user;
  if(empty($account)){
    $account = $user;
  }
  if(user_access('pg_account operator',$user) && $user->uid != $account->uid){
    return drupal_get_form('pg_account_addFundsByOperator',$account);
  }else{
    return drupal_get_form('pg_account_addFunds');
  }
}

function pg_account_account_withdraw($account){
  global $user;
  if(empty($account)){
    $account = $user;
  }
  if(user_access('pg_account operator',$user)){
    return drupal_get_form('pg_account_withdrawByOperator',$account);
  }else{
    return drupal_get_form('pg_account_withdraw');
  }
}


function pg_account_addFundsByOperator($form,&$form_state,$account){

  if(!empty($account)){
    $form['uid'] = array(
      '#type' => 'hidden',
      '#value' => $account->uid,
    );
  }else{
    $options=array();
    $result = db_query('SELECT u.uid,name,balance FROM {users} u LEFT JOIN {pg_account_balance} b ON b.uid= u.uid WHERE u.uid > 0');
    while($account = db_fetch_object($result)){
      $options[$account->uid]= $account->name.' '.pgapi_format_price_plain('pg_account',$account->balance,pg_account_getCurrentSymbol($account));
    }
    
    $form['uid'] = array(
      '#type' => 'select',
      '#title' => t('User'),
      '#options' => $options,
    );  
  
  }


  $form['amount'] = array(
    '#type'  => 'textfield',
    '#title' => t('Amount'),
    '#required'  => TRUE,
  );
  
  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description'),
    '#required'  => TRUE,
  );
    
  $form['submit'] = array(
    '#type'  => 'submit',
    '#value' => t('Continue'),
  );
  return $form;

}

function pg_account_addFundsByOperator_validate($form, &$form_state) {
  global $user;

  if($form_state['values']['amount']<=0){
    form_set_error('amount', t('Amount must be more than 0.'));
  }
  
}

function pg_account_addFundsByOperator_submit($form, &$form_state){
  global $user;

  $account = user_load($form_state['values']['uid']);
  
  $data = new stdClass();
  $data->amount = $form_state['values']['amount'];
  
  $data->description=$form_state['values']['description']."\n".t('Processed by !user',array('!user'=>$user->name));
  
  $data->uid=$form_state['values']['uid'];
  
  
  //create transactions
  $transaction = new stdClass();
    
  $transaction->amount = $data->amount;

  $transaction->gateway = 'pg_account';
  $transaction->service = 'pg_account';

  
  if(!isset($data->uid)){
    $data->uid = $user->uid;
  }
  $transaction->uid = $data->uid;
  
  if(!isset($data->sender)){
    $data->sender = $user->uid;
  }
  
  if(!isset($data->title)){
    $data->title = t('Add Funds for !user',array('!user' => $account->name));
  }
  $transaction->title=$data->title;

  $transaction->description=$data->description;  

  pgapi_transaction_save($transaction);
  
  //the same to pg_account_transaction
  $taccount = new stdClass();
  $taccount->amount = $data->amount;
  $taccount->operation = PG_ACCOUNT_ADD_BALANCE;
  $taccount->uid = $data->uid;
  $taccount->sender = $data->sender;
  $taccount->pgapi_txnid = $transaction->txnid;
  pg_account_transaction_save($taccount);
  
  $data->txnid = $taccount->txnid;
  
  if(pg_account_balance_add($data)){
    $transaction->status = pgapi_get_status_id('completed');
    $transaction->workflow = pgapi_get_workflow_id('completed');
    
    pgapi_transaction_save($transaction);
    drupal_set_message(t('You added  !formated_amount to !user account.', array(
      '!user' => $account->name,
      '!formated_amount' => pgapi_format_price_plain($data->amount))));


    //create tokens data
    $tokens['username'] = $account->name;
    $tokens['operatorname'] = $user->name;
    $tokens['description'] = $form_state['values']['description'];
    $tokens['amount'] = pgapi_format_price_plain($data->amount);
    
    $balance=pg_account_balance_load($account->uid);
    $tokens['balance'] = pgapi_format_price_plain($balance->balance);
                  
    $roles = variable_get('pg_account_roles',array());
    if(!empty($roles)){
      $sent_uids = array();
      foreach($roles as $rid => $val){
        if($val){
          //get users
          $users = db_select('users_roles', 'ur')
          ->fields('ur',array('uid'))
          ->condition('ur.rid',$rid)
          ->execute()
          ->fetchAll();
          foreach($users as $u){
            if(!in_array($u->uid,$sent_uids)){
              $sent_uids[]=$u->uid;
  
              //send notice
              $notice = new stdClass();
              $notice->uid = $u->uid;
              $notice->provider = 'pg_account'; 
              
              $message = t(variable_get('pg_account_admin_message_add_body',PG_ACCOUNT_ADD_BALANCE_ADMIN_MESSAGE));
              
              $notice->message = token_replace($message, array('pg_account' => $tokens));

              $subject = t(variable_get('pg_account_admin_title_add',PG_ACCOUNT_ADD_BALANCE_ADMIN_TITLE));
              
              $notice->subject = token_replace($subject, array('pg_account' => $tokens));

              $notice->format = variable_get('pg_account_admin_message_add_format','filtered_html');
              notices_save($notice);
            }
          }
        }
      }
    }
    
    $notice = new stdClass();
    $notice->uid = $data->uid;
    $notice->provider = 'pg_account'; 
    $message = t(variable_get('pg_account_user_message_add_body',PG_ACCOUNT_ADD_BALANCE_USER_MESSAGE));
              
    $notice->message = token_replace($message, array('pg_account' => $tokens));

    $subject = t(variable_get('pg_account_user_title_add',PG_ACCOUNT_ADD_BALANCE_USER_TITLE));
    
    $notice->subject = token_replace($subject, array('pg_account' => $tokens));

    $notice->format = variable_get('pg_account_user_message_add_format','filtered_html');
    
    notices_save($notice);
    
    drupal_goto('/user/'.$account->uid.'/balance'); 
  }
}


function pg_account_withdrawByOperator($form,&$form_state,$account){
  if(!empty($account)){
    $form['uid'] = array(
      '#type' => 'hidden',
      '#value' => $account->uid,
    );
  }else{
    $options=array();
    $result = db_query('SELECT u.uid,name,balance FROM {users} u LEFT JOIN {pg_account_balance} b ON b.uid= u.uid WHERE u.uid > 0');
    while($account = db_fetch_object($result)){
      $options[$account->uid]= $account->name.' '.pgapi_format_price_plain('pg_account',$account->balance,pg_account_getCurrentSymbol($account));
    }
    
    $form['uid'] = array(
      '#type' => 'select',
      '#title' => t('User'),
      '#options' => $options,
    );  
  
  }
  
  $form['amount'] = array(
    '#type'  => 'textfield',
    '#title' => t('Amount'),
    '#required'  => TRUE,
  );
  
  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description'),
    '#required'  => TRUE,
  );
    
  $form['submit'] = array(
    '#type'  => 'submit',
    '#value' => t('Continue'),
  );
  return $form;

}

function pg_account_withdrawByOperator_validate($form, &$form_state) {
  global $user;
  $balance = pg_account_balance_load($form_state['values']['uid']);

  if($form_state['values']['amount']<=0){
    form_set_error('amount', t('Amount must be more than 0.'));
  }
  
  if($form_state['values']['amount'] > $balance->balance) {
    form_set_error('amount', t('You tried to withdraw !formated_amount. You cannot withdraw more than !formated_balance.', array('!formated_balance' => pgapi_format_price('pg_account',$balance->balance,pg_account_getCurrentSymbol($account) ), '!formated_amount'=>pgapi_format_price('pg_account',$form_state['values']['amount'],pg_account_getCurrentSymbol($account) ))));
  }
}

function pg_account_withdrawByOperator_submit($form, &$form_state){
  global $user;

  $account = user_load($form_state['values']['uid']);
  
  $data = new stdClass();

  $data->amount=$form_state['values']['amount'];

  $data->description=$form_state['values']['description']."\n".t('Processed by !user',array('!user'=>$user->name));
  
  $data->uid=$form_state['values']['uid'];
  
  //create transactions
  $transaction = new stdClass();
    
  $transaction->amount = (0-$data->amount);

  $transaction->gateway = 'pg_account';
  $transaction->service = 'pg_account';

  
  if(!isset($data->uid)){
    $data->uid = $user->uid;
  }
  $transaction->uid=$data->uid;
  
  if(!isset($data->sender)){
    $data->sender = $user->uid;
  }
  
  if(!isset($data->title)){
    $data->title = t('Withdraw funds from !user',array('!user' => $account->name));
  }
  $transaction->title=$data->title;

  $transaction->description=$data->description;  

  pgapi_transaction_save($transaction);
  
  //the same to pg_account_transaction
  $taccount = new stdClass();
  $taccount->amount = (0 - $data->amount);
  $taccount->operation = PG_ACCOUNT_SUB_BALANCE;
  $taccount->uid = $data->uid;
  $taccount->sender = $data->sender;
  $taccount->pgapi_txnid = $transaction->txnid;
  pg_account_transaction_save($taccount);
  
  $data->txnid = $taccount->txnid;
  
  if(pg_account_balance_sub($data)){
    $transaction->status = pgapi_get_status_id('completed');
    $transaction->workflow = pgapi_get_workflow_id('completed');
    
    pgapi_transaction_save($transaction);
    drupal_set_message(t('You withdraw !formated_amount from !user account.', array(
      '!user' => $account->name,
      '!formated_amount' => pgapi_format_price_plain($data->amount))));
    
    //create tokens data
    $tokens['username'] = $account->name;
    $tokens['operatorname'] = $user->name;
    $tokens['description'] = $form_state['values']['description'];
    $tokens['amount'] = pgapi_format_price_plain($data->amount);
    
    $balance=pg_account_balance_load($account->uid);
    $tokens['balance'] = pgapi_format_price_plain($balance->balance);

    $roles = variable_get('pg_account_roles',array());
    if(!empty($roles)){
      $sent_uids = array();
      foreach($roles as $rid => $val){
        if($val){
          //get users
          $users = db_select('users_roles', 'ur')
          ->fields('ur',array('uid'))
          ->condition('ur.rid',$rid)
          ->execute()
          ->fetchAll();
          foreach($users as $u){
            if(!in_array($u->uid,$sent_uids)){
              $sent_uids[]=$u->uid;
  
              //send notice
              $notice = new stdClass();
              $notice->uid = $u->uid;
              $notice->provider = 'pg_account'; 
              
              $message = t(variable_get('pg_account_admin_message_sub_body',PG_ACCOUNT_SUB_BALANCE_ADMIN_MESSAGE));
              
              $notice->message = token_replace($message, array('pg_account' => $tokens));

              $subject = t(variable_get('pg_account_admin_title_sub',PG_ACCOUNT_SUB_BALANCE_ADMIN_TITLE));
              
              $notice->subject = token_replace($subject, array('pg_account' => $tokens));

              $notice->format = variable_get('pg_account_admin_message_sub_format','filtered_html');
              notices_save($notice);
            }
          }
        }
      }
    }
    
    //send notice to user
    $balance=pg_account_balance_load($account->uid);
    
    $notice = new stdClass();
    $notice->uid = $data->uid;
    $notice->provider = 'pg_account'; 
    $message = t(variable_get('pg_account_user_message_sub_body',PG_ACCOUNT_SUB_BALANCE_USER_MESSAGE));
              
    $notice->message = token_replace($message, array('pg_account' => $tokens));

    $subject = t(variable_get('pg_account_user_title_sub',PG_ACCOUNT_SUB_BALANCE_USER_TITLE));
    
    $notice->subject = token_replace($subject, array('pg_account' => $tokens));

    $notice->format = variable_get('pg_account_user_message_sub_format','filtered_html');
    
    notices_save($notice);
    
  }else{
    drupal_set_message(t('We could not process your request.%user doesn\'t have enough money.', array( '%user' => $account->name)));
  }
  drupal_goto('/user/'.$account->uid.'/balance'); 

}


function pg_account_addFunds($form, $form_state = NULL){
  if(isset($form_state['t'])){
    $form = pgapi_payment($form,$form_state,$form_state['t']);
//    $form['#theme'] = 'pgapi_payment';
    $form['#submit'] = array('pgapi_payment_submit');
  }else{
    $form['amount'] = array(
      '#type'  => 'textfield',
      '#title' => t('Amount'),
      '#required'  => TRUE,
    );
    
    $form['actions'] = array('#type' => 'actions');
    $form['actions']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Continue'),
    );
    
  }
  return $form;
}

function pg_account_addFunds_validate($form, &$form_state) {
  if(!isset($form_state['values']['txnid']) && $form_state['values']['amount'] < 0.01){
    form_set_error('amount', t('Amount must be more than 0.009 .'));
  }
}

function pg_account_addFunds_submit($form, &$form_state){
  global $user;
  if(empty($form_state['values']['txnid'])){
    $transaction = new stdClass();
    $amount = $form_state['values']['amount'];
    
    $transaction->amount = $amount;
    $transaction->service='pg_account';
    $transaction->uid = $user->uid;
    $transaction->title = t('Add Balance funds for !user',array('!user' => $user->name));
    $transaction->description = '';
    pgapi_transaction_save($transaction);
    
    //the same to pg_account_transaction
    $taccount = new stdClass();
    $taccount->amount = $amount; //withdraw spuppost to be less than 0;
    $taccount->operation = PG_ACCOUNT_ADD_BALANCE;    
    $taccount->uid = $user->uid;
    $taccount->pgapi_txnid = $transaction->txnid;
    pg_account_transaction_save($taccount);

    $form_state['t'] = $transaction;
    $form_state['rebuild'] = TRUE;
  }else{
    
    unset($form_state['t']);
    $form_state['rebuild'] = TRUE;
  }
}

function pg_account_account_send($form, &$form_state, $from_user){

  $form_state['uid'] =  $from_user->uid;

  $form['amount'] = array(
    '#type'  => 'textfield',
    '#title' => t('Amount'),
    '#required'  => TRUE,
  );

  $form['send_to'] = array(
    '#type' => 'textfield',
    '#title' => t('Username'),
    '#default_value' => '',
    '#autocomplete_path' => 'user/autocomplete',
    '#description' => t('Please enter username'),
    '#required'  => TRUE,    
  );  
  
  $form['submit'] = array(
    '#type'  => 'submit',
    '#value' => t('Continue'),
  );
  return $form;
}

function pg_account_account_send_validate($form, &$form_state) {
  global $user;
  $balance = pg_account_balance_load($user->uid);
  if($form_state['values']['amount']< 0.01){
    form_set_error('amount', t('Amount must be more than 0.'));
  }
  
  if ($send_to_account = user_load_by_name($form_state['values']['send_to'])) {
//    if($user->uid == $send_to_account->uid OR $form_state['uid'] == $send_to_account->uid){
    if($form_state['uid'] == $send_to_account->uid){
      form_set_error('send_to', t('Please enter another account name.'));
    }
  }else{
    form_set_error('send_to', t('User does not exist.'));
  }
  
  if($form_state['values']['amount'] > $balance->balance) {
    form_set_error('amount', t('You tried to withdraw !formated_amount. You cannot withdraw more than !formated_balance.', array(
      '!formated_balance' => pgapi_format_price_plain($balance->balance),
      '!formated_amount' => pgapi_format_price_plain($form_state['values']['amount']))));
  }
}

function pg_account_account_send_submit($form, &$form_state) {
  global $user;
  
  //withdraw first
  $account = user_load($form_state['uid']);
  $send_to_account = user_load_by_name($form_state['values']['send_to']);
  $data = new stdClass();

  $data->amount = $form_state['values']['amount'];

  $text = t('Send money to !user',array('!user'=>$send_to_account->name));
  if($account->uid != $user->uid){
    $data->description = $data->title = $text."\n".t('Processed by !user',array('!user'=>$user->name));
  }else{
    $data->description = $data->title = $text;    
  }
  
  $data->uid = $account->uid;
  
  //create transactions
  $transaction = new stdClass();
    
  $transaction->amount = (0-$data->amount);

  $transaction->gateway = 'pg_account';
  $transaction->service = 'pg_account';

  
  if(!isset($data->uid)){
    $data->uid = $user->uid;
  }
  $transaction->uid=$data->uid;
  
  if(!isset($data->sender)){
    $data->sender = $user->uid;
  }
  
  if(!isset($data->title)){
    $data->title = t('Withdraw funds from !user',array('!user' => $account->name));
  }
  $transaction->title=$data->title;

  $transaction->description=$data->description;  

  pgapi_transaction_save($transaction);
  
  //the same to pg_account_transaction
  $taccount = new stdClass();
  $taccount->amount = (0 - $data->amount);
  $taccount->operation = PG_ACCOUNT_SUB_BALANCE;
  $taccount->uid = $data->uid;
  $taccount->sender = $data->sender;
  $taccount->pgapi_txnid = $transaction->txnid;
  pg_account_transaction_save($taccount);
  
  $data->txnid = $taccount->txnid;
  
  if(pg_account_balance_sub($data)){
    $transaction->status = pgapi_get_status_id('completed');
    $transaction->workflow = pgapi_get_workflow_id('completed');
    
    pgapi_transaction_save($transaction);
    
    //create tokens data
    $tokens['username'] = $account->name;
    $tokens['username_received'] = $send_to_account->name;
    $tokens['description'] = $data->description;
    $tokens['amount'] = pgapi_format_price_plain($data->amount);
    
    $balance=pg_account_balance_load($account->uid);
    $tokens['balance'] = pgapi_format_price_plain($balance->balance);

    //send notice
    $notice = new stdClass();
    $notice->uid = $data->uid;
    $notice->provider = 'pg_account'; 
    $message = t(variable_get('pg_account_user_message_send_body',PG_ACCOUNT_SEND_BALANCE_USER_MESSAGE));
              
    $notice->message = token_replace($message, array('pg_account' => $tokens));

    $subject = t(variable_get('pg_account_user_title_send',PG_ACCOUNT_SEND_BALANCE_USER_TITLE));
    
    $notice->subject = token_replace($subject, array('pg_account' => $tokens));

    $notice->format = variable_get('pg_account_user_message_send_format','filtered_html');
    
    notices_save($notice);
        
    //add balance to $send_to_account
    $data = new stdClass();
    $data->amount = $form_state['values']['amount'];
    
    $data->description = $data->title = t('Received money from !user',array('!user' => $account->name));
    
    $data->uid = $send_to_account->uid;

    //create transactions
    $transaction = new stdClass();
      
    $transaction->amount = $data->amount;
  
    $transaction->gateway = 'pg_account';
    $transaction->service = 'pg_account';
  
    
    if(!isset($data->uid)){
      $data->uid = $user->uid;
    }
    $transaction->uid=$data->uid;
    
    if(!isset($data->sender)){
      $data->sender = $account->uid;
    }
    
    if(!isset($data->title)){
      $data->title = t('Add Funds for !user',array('!user' => $send_to_account->name));
    }
    $transaction->title=$data->title;
  
    $transaction->description=$data->description;  
  
    pgapi_transaction_save($transaction);
    
    //the same to pg_account_transaction
    $taccount = new stdClass();
    $taccount->amount = $data->amount;
    $taccount->operation = PG_ACCOUNT_ADD_BALANCE;
    $taccount->uid = $data->uid;
    $taccount->sender = $data->sender;
    $taccount->pgapi_txnid = $transaction->txnid;
    pg_account_transaction_save($taccount);
    
    $data->txnid = $taccount->txnid;
    
    if(pg_account_balance_add($data)){
      $transaction->status = pgapi_get_status_id('completed');
      $transaction->workflow = pgapi_get_workflow_id('completed');
      
      pgapi_transaction_save($transaction);
      
      //create tokens data
      $tokens['username'] = $account->name;
      $tokens['username_received'] = $send_to_account->name;
      $tokens['description'] = $data->description;
      $tokens['amount'] = pgapi_format_price_plain($data->amount);
      
      $balance=pg_account_balance_load($send_to_account->uid);
      $tokens['balance'] = pgapi_format_price_plain($balance->balance);
  
      //send notice
      $notice = new stdClass();
      $notice->uid = $data->uid;
      $notice->provider = 'pg_account'; 
      $message = t(variable_get('pg_account_user_message_receive_body',PG_ACCOUNT_RECEIVE_BALANCE_USER_MESSAGE));
                
      $notice->message = token_replace($message, array('pg_account' => $tokens));
  
      $subject = t(variable_get('pg_account_user_title_receive',PG_ACCOUNT_RECEIVE_BALANCE_USER_TITLE));
      
      $notice->subject = token_replace($subject, array('pg_account' => $tokens));
  
      $notice->format = variable_get('pg_account_user_message_receive_format','filtered_html');

      notices_save($notice);    
    }

    
  }else{
    drupal_set_message(t('We could not process your request.%user doesn\'t have enough money.', array( '%user' => $account->name)));
  }
  drupal_goto('/user/'.$form_state['uid'].'/balance'); 
}

function pg_account_withdraw($form,&$form_state){
  $options=pg_account_getActiveMoneyOutMethod();
  if(!empty($options)){
    $default_method = key($options);
	  $form['amount'] = array(
	    '#type'  => 'textfield',
	    '#title' => t('Amount (!currency)',array('!currency'=>pg_account_getCurrentSymbol())),
      '#required'  => TRUE,
	  );
	  
	  $form['method'] = array(
	    '#title'   => t('Transfer to'),
	    '#type'    => 'select',
	    '#options' => $options,
	    '#ahah' => array(
        'path'    => 'pgaccount/withdraw_wrapper_js',
        'wrapper' => 'moneyout-wrapper-process',
        'method'  => 'replace',
        'effect'  => 'slide',
       ),
	  );
	  
	  $form['method_details'] = array(
	    '#tree'  => true,
      '#prefix' => '<div id="moneyout-wrapper-process">',
      '#suffix' => '</div>',
	  );
	  
  	if(!empty($default_method)){
      $extra_form=module_invoke($default_method, 'pgaccount_withdraw','get form', $form_state);
      if(is_array($extra_form)){
        $form['method_details'] += $extra_form;
      }
    } 
  
	  $form['submit'] = array(
	    '#type'  => 'submit',
	    '#value' => t('Continue'),
	  );
  }else{
    //if has access 'administer pg_account'
    if (user_access('administer pg_account')) {
      $message= t('No one money widthdraw method enabled. Go to !link and enable it.',array('!link' => l(t('Withdraw settings'),'admin/pgdata/pgaccount_settings')));
    }else{
      $site_mail =variable_get('site_mail', ini_get('sendmail_from'));
      $message= t('Please send mail to !site_mail to enable money withdraw for your account.',array( '!site_mail' => '<a href="mailto:' . $site_mail . '">' . $site_mail . '</a>'));
    } 
	  $form['message'] = array(
	    '#type'  => 'markup',
	    '#value' => $message,
	  );
  }
  return $form;
}


function pg_account_withdraw_validate($form, &$form_state) {
  global $user;
  $balance = pg_account_balance_load($user->uid);

  if($form_state['values']['amount']< 0.01){
    form_set_error('amount', t('Amount must be more than 0.'));
  }
  
  if($form_state['values']['amount'] > $balance->balance) {
    form_set_error('amount', t('You tried to withdraw !formated_amount. You cannot withdraw more than !formated_balance.', array('!formated_balance'=>pgapi_format_price('pg_account',$balance->balance,pg_account_getCurrentSymbol($user) ),'!formated_amount'=>pgapi_format_price('pg_account',$form_state['values']['amount'],pg_account_getCurrentSymbol($user) ))));
  }
}

function pg_account_withdraw_submit($form, &$form_state){
  global $user;

  $data = (object)(NULL);
  $data->amount=$form_state['values']['amount'];
  $data->method=$form_state['values']['method'];
  $data->extra=$form_state['values']['method_details'];
  $data->uid=$user->uid;
  
  if(pg_account_balance_sub($data)){
    drupal_set_message(t('Our operators will process your request shortly.'));
  }else{
    drupal_set_message(t('We could not process your request.You don\'t have enough money.'));
  }

}

