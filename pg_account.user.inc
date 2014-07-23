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
  
  //$filters = formfilterapi_build_filter_query('pg_account_personal_history');
  $header = array(
    'date' => array('data' => t('Date'), 'field' => 'pgt.changed', 'sort' => 'desc'),
    'amount' => array('data' => t('Amount'), 'field' => 'pgt.amount'),
    'description' => array('data' => t('Description')),
    'status' => array('data' => t('Payment status'), 'field' => 'pgt.status'),
    'workflow' => array('data' => t('Operation status'), 'field' => 'pgt.workflow'),

  );
/*  if(empty($filters['where'])){
    $filters['where'] = "WHERE t.uid='".$account->uid."'";
  } else {
    $filters['where'] .= " AND t.uid='".$account->uid."'";
  }
*/  
  $query = db_select('pg_account_transaction', 'pat')->extend('AltPager')->extend('TableSort');
  $query->fields('pat');    
  $query->leftjoin('pgapi_transaction', 'pgt', 'pat.pgapi_txnid = %alias.txnid');
  $query->fields('pgt');    
  $query->condition('pat.uid', $account->uid);
  $query->extend('TableSort');
  $query->orderByHeader($header);
  
  $transactions= $query->execute()->fetchAll();

  foreach ($transactions as $txn) {

    $description = $txn->description ? $txn->description  : $txn->title;

    if($txn->status == PG_CANCELED){
        $description .= $txn->title.'<br>'.$txn->description;
    }

    $class = array($txn->operation, 'status-'.$txn->status, 'workflow-'.$txn->workflow );    
    
    $rows[] = array(
      'data' => array(
        array( 'data' => format_date($txn->changed,'custom','M d, Y'), 'class' => array('date')),
        array( 'data' => pgapi_format_price($txn->amount), 'class' => array('amount')),
        $description,
        pgapi_get_status($txn->status),
        pgapi_get_workflow($txn->workflow),
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
  if(user_access('pg_account operator',$user)){
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
  $transaction->uid=$data->uid;
  
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
    
    drupal_set_message($transaction->title);
    //drupal_set_message(t('You added !formated_amount to !user account.',array('!user' => $account->name,'!formated_amount'=>pgapi_format_price('pg_account',$form_state['values']['amount'],pg_account_getCurrentSymbol($account) ))));
    /*
    $language = user_preferred_language($account);
    $variables = user_mail_tokens($account, $language);
    
    $variables['!formated_amount'] =  pgapi_format_price_plain('pg_account',$form_state['values']['amount'],pg_account_getCurrentSymbol($account) );
  	$variables['!operatorname'] = $user->name;
  	$variables['!reason'] = $form_state['values']['description'];
  	
  	$balance=pg_account_balance_load($account->uid);
  	$variables['!balance'] = pgapi_format_price('pg_account',$balance->balance,pg_account_getCurrentSymbol($account));
  	
  	  	
    $notice->uid = $account->uid;
    $notice->teaser = t('Balance updated by !operatorname. New balance is !balance', $variables, $language->language);
    $notice->body = t("Dear !username
Operator !operatorname added !formated_amount to your balance with comment :
!reason

Your current Balance: !balance
", $variables, $language->language);
    $notice->callback = 'pg_account';
    
    notice_save($notice);
    
    $admin_uid=variable_get('pg_account_admin_uid','1');
    if(!empty($admin_uid)){
      //notify admin about balance update
      $noticeadmin->uid=$admin_uid;
      $noticeadmin->teaser = t('Add !formated_amount funds to !username by !operatorname', $variables, $language->language);
      $noticeadmin->body = t("Operator !operatorname added !formated_amount to !username balance with comment :
  !reason
  
  New balance is: !balance
  ", $variables, $language->language);
      $noticeadmin->callback = 'pg_account';
      notice_save($noticeadmin);
    }    */
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
  
    drupal_set_message($transaction->title);
/*    drupal_set_message(t('You withdraw !formated_amount from !user account.',array('!formated_amount'=>pgapi_format_price('pg_account',$form_state['values']['amount'],pg_account_getCurrentSymbol($account) ), '!user' => theme('username',$account))));

    $language = user_preferred_language($account);
    $variables = user_mail_tokens($account, $language);
    
    $variables['!formated_amount'] =  pgapi_format_price_plain('pg_account',$form_state['values']['amount'],pg_account_getCurrentSymbol($account) );
  	$variables['!operatorname'] = $user->name;
  	$variables['!reason'] = $form_state['values']['description'];
  	
  	$balance=pg_account_balance_load($account->uid);
  	$variables['!balance'] = pgapi_format_price('pg_account',$balance->balance,pg_account_getCurrentSymbol($account));
  	
  	  	
    $notice->uid = $account->uid;
    $notice->teaser = t('Balance updated by !operatorname. New balance is !balance', $variables, $language->language);
    $notice->body = t("Dear !username
Operator !operatorname withdraw !formated_amount from your balance with comment :
!reason

Your current Balance: !balance
", $variables, $language->language);
    $notice->callback = 'pg_account';
    
    notice_save($notice);
    
    $admin_uid=variable_get('pg_account_admin_uid','1');
    if(!empty($admin_uid)){
      //notify admin about balance update
      $noticeadmin->uid=$admin_uid;
      $noticeadmin->teaser = t('Withdraw !formated_amount funds from !username by !operatorname', $variables, $language->language);
      $noticeadmin->body = t("Operator !operatorname withdraw !formated_amount from !username balance with comment :
  !reason
  
  New balance is: !balance
  ", $variables, $language->language);
      $noticeadmin->callback = 'pg_account';
      notice_save($noticeadmin);
    } */
    
  }else{
    drupal_set_message(t('We could not process your request.%user doesn\'t have enough money.', array( '%user' => $account->name)));
  }
  drupal_goto('/user/'.$account->uid.'/balance'); 

}


function pg_account_addFunds($form, $form_state = NULL){
  if(isset($form_state['t'])){
    module_load_include('inc', 'pgapi', 'pgapi.pages');
    $form = pgapi_payment($form,$form_state,$form_state['t']);
//    $form['#theme'] = 'pgapi_payment';
    $form['#submit'] = array('pgapi_payment_submit');
  }else{
    $form['amount'] = array(
      '#type'  => 'textfield',
      '#title' => t('Amount (!currency)',array('!currency'=>pg_account_getCurrentSymbol())),
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
    $transaction->title = t('Add Funds to account for !user',array('!user' => $user->name));
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

function pg_account_account_send($form, &$form_state){

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
  );  
  
  $form['submit'] = array(
    '#type'  => 'submit',
    '#value' => t('Continue'),
  );
  return $form;
}

function pg_account_account_send_validate($form, &$form_state) {
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

