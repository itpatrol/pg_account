<?php
// $Id: pg_account.admin.inc,v 1.2 2010/10/30 23:09:08 gor Exp $

/********************************************************************
 * Functions callback
 ********************************************************************/

function pg_account_history(){
  $filters = formfilterapi_build_filter_query('pg_account_operator_history');
  $header = array(
    array('data' => t('Date'), 'field' => 't.changed', 'sort' => 'desc'),
    array('data' => t('Amount'), 'field' => 't.amount'),
    array('data' => t('User'), 'field' => 'u.name'),
    array('data' => t('Description')),
    array('data' => t('Payment status'), 'field' => 't.status'),
    array('data' => t('Operation status'), 'field' => 't.workflow'),    
    t('Operations')
  );

  $sql='SELECT t.*,u.name FROM {pg_account_transaction} t LEFT JOIN {users} u ON u.uid=t.uid '.$filters['join'].' '.$filters['where'].tablesort_sql($header);

  $pagerCountShow=altpager_Show($sql,$filters['args']);
  $count = altpager_getCount();
  
  $result = db_query_range($sql,$filters['args'],0,$count);
  
  while ($txn = db_fetch_object($result)) {
    //operations with transaction
    $actions =array();
    
    if($txn->method != '' && $txn->status == PG_RECEIVED && $txn->workflow == PG_WORKFLOW_RECEIVED ){
      $actions[]=array(
        'href'  => 'admin/reports/pgaccount_transactions/'.$txn->txnid.'/send',
        'title' => t('Send money'),
        'query' => drupal_get_destination(),
      );
      $actions[]=array(
        'href'  => 'admin/reports/pgaccount_transactions/'.$txn->txnid.'/cancel',
        'title' => t('Cancel transaction'),
        'query' => drupal_get_destination(),
      );
    }
    if($txn->method != '' && $txn->status == PG_COMPLETED && $txn->workflow == PG_WORKFLOW_COMPLETED  && $txn->amount < 0){
      $actions[]=array(
        'href'  => 'admin/reports/pgaccount_transactions/'.$txn->txnid.'/refund',
        'title' => t('Refund money'),
        'query' => drupal_get_destination(),
      );
    }

    if($txn->method != '' && $txn->status == PG_COMPLETED && $txn->workflow == PG_WORKFLOW_COMPLETED  && $txn->amount > 0){
      $actions[]=array(
        'href'  => 'admin/reports/pgaccount_transactions/'.$txn->txnid.'/cancel',
        'title' => t('Cancel transaction'),
        'query' => drupal_get_destination(),
      );
    }
    
    //style stuff
    $class='received';
    if($txn->amount < 0 ){
      $class='sent';
      if($txn->pgapi_txnid){
        //payment
        $t=pgapi_transaction_load($txn->pgapi_txnid);
        $description=$t->description;
      }else{
        //withdraw
        $description=t('Withdraw via %paymentsystem ', array('%paymentsystem' => module_invoke($txn->method, 'pgaccount_withdraw','display name')));
      }
    }else{
      if($txn->pgapi_txnid){
        //add funds
        $t=pgapi_transaction_load($txn->pgapi_txnid);
        $description=$t->description ? $t->description  : $t->title;
      }else{
        //withdraw
        $description=$txn->description;
      }
    }

    if($txn->method == 'manual'){
      $description=$txn->description;
    }
    
    $class .= ' status-'.$txn->status.' workflow-'.$txn->workflow;
    
    $account = user_load($txn->uid);
    
    $rows[] = array(
      'data'=>array(
        array( 'data' => format_date($txn->changed,'custom','M d, Y'), 'class' => 'date'),
        array( 'data' => pgapi_format_price('pg_account',$txn->amount,pg_account_getCurrentSymbol($account)), 'class' => 'amount'),
        theme('username',$account),
        $description,
        pgapi_get_status($txn->status),
        pgapi_get_workflow($txn->workflow),
        theme('links',$actions,array('class' => 'actions')),
      ),
      'class' => $class,
    );

  }//while
  if(empty($rows)){
    $rows[] = array(array('data' => t('No data available.'), 'colspan' => '7'));
  }
  $output .= $pagerCountShow;
  $output .= '<div width=99% style="overflow-x: auto; ">';
  $output .=  theme("table", $header, $rows);
  $output .= "</div>";
  $output .= $pagerCountShow;
  
  
  return $output;

}

function pg_account_personal_history($account){
  if(empty($account)){
    global $user;
    $account = $user;
  }
  
  $filters = formfilterapi_build_filter_query('pg_account_personal_history');
  $header = array(
    array('data' => t('Date'), 'field' => 't.changed', 'sort' => 'desc'),
    array('data' => t('Amount'), 'field' => 't.amount'),
    array('data' => t('Description')),
    array('data' => t('Payment status'), 'field' => 't.status'),
    array('data' => t('Operation status'), 'field' => 't.workflow'),

  );
  if(empty($filters['where'])){
    $filters['where'] = "WHERE t.uid='".$account->uid."'";
  } else {
    $filters['where'] .= " AND t.uid='".$account->uid."'";
  }
  $sql='SELECT t.* FROM {pg_account_transaction} t '.$filters['join'].' '.$filters['where'].tablesort_sql($header);

  
  $pagerCountShow=altpager_Show($sql,$filters['args']);
  $count = altpager_getCount();
  
  $result = db_query_range($sql,$filters['args'],0,$count);
  
  while ($txn = db_fetch_object($result)) {
    $class='received';
    if($txn->amount < 0 ){
      $class='sent';
      if($txn->pgapi_txnid){
        //payment
        $t=pgapi_transaction_load($txn->pgapi_txnid);
        $description=$t->description;
      }else{
        //withdraw
        $description=t('Withdraw via %paymentsystem ', array('%paymentsystem' => module_invoke($txn->method, 'pgaccount_withdraw','display name')));
      }
    }else{
      if($txn->pgapi_txnid){
        //add funds
        $t=pgapi_transaction_load($txn->pgapi_txnid);
        $description=$t->description ? $t->description  : $t->title;
        if($txn->status == PG_CANCELED){
          $description .= $t->description.'<br>'.$txn->description;
        }

      }else{
        //withdraw
        $description=$txn->description;
      }
    }

    if($txn->method == 'manual'){
      $description=$txn->description;
    }
    
    
    $class .= ' status-'.$txn->status.' workflow-'.$txn->workflow;
    
    
    $rows[] = array(
      'data'=>array(
        array( 'data' => format_date($txn->changed,'custom','M d, Y'), 'class' => 'date'),
        array( 'data' => $txn->amount, 'class' => 'amount'),
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
  $output .= $pagerCountShow;
  $output .= '<div width=99% style="overflow-x: auto; ">';
  $output .=  theme("table", $header, $rows);
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
  
  $count=1;
  if(module_exists('locale') ){
    $lang_list = language_list('enabled');
    $count = count($lang_list[1]);
  }

  if($count > 1 ){
    $default = language_default();
    $langs=variable_get('pg_account_rates',array());
    foreach($lang_list[1] as $key => $lang){
      if(empty($langs[$key])){
        if($key == $default->language){
          $langs[$key]['pg_account_rate']= variable_get('pg_account_rate','1.00');   
          $langs[$key]['pg_account_symbol']=variable_get('pg_account_symbol','$');
          $langs[$key]['pg_account_decimal_places']=variable_get('pg_account_decimal_places',2);
          $langs[$key]['pg_account_decimal']=variable_get('pg_account_decimal','.');
          $langs[$key]['pg_account_thousands']=variable_get('pg_account_thousands',',');
          $langs[$key]['pg_account_symbol_position']= variable_get('pg_account_symbol_position',1);
        }else{
          $langs[$key]['pg_account_rate']= '';   
          $langs[$key]['pg_account_symbol']='';
          $langs[$key]['pg_account_decimal_places']='';
          $langs[$key]['pg_account_decimal']='';
          $langs[$key]['pg_account_thousands']='';
          $langs[$key]['pg_account_symbol_position']= '';
        }
      }

      $form[$key]=array(
        '#type' => 'fieldset',
        '#title' => t('Currency setting for !lang', array('!lang' => $lang->name)),
        '#tree' => TRUE,
      );
      
      $form[$key]['pg_account_rate'] = array(
        '#type'          => 'textfield',
        '#title'         => t('Rate to site currency'),
        '#default_value' => $langs[$key]['pg_account_rate'],
        '#description'   => t("Please enter Account Balance rate according to site currency."),
        '#required'      => true,
      );
      $form[$key]['pg_account_symbol'] = array(
        '#type'          => 'textfield',
        '#title'         => t('Symbol'),
        '#default_value' => $langs[$key]['pg_account_symbol'],
        '#description'   => t("Please enter symbol of the Account Balance currency."),
        '#required'      => true,
      );
      
    
      $form[$key]['pg_account_decimal_places'] = array(
        '#type'          => 'textfield',
        '#title'         => t('Decimal places'),
        '#default_value' => $langs[$key]['pg_account_decimal_places'],
        '#description'   => t("Please enter number of decimal places."),
        '#required'      => true,
      );
  
      $form[$key]['pg_account_decimal'] = array(
        '#type'          => 'textfield',
        '#title'         => t('Decimal delimiter'),
        '#default_value' => $langs[$key]['pg_account_decimal'],
        '#description'   => t("Please enter decimal delimiter."),
        '#required'      => true,
      );
  
      $form[$key]['pg_account_thousands'] = array(
        '#type'          => 'textfield',
        '#title'         => t('Thousands places'),
        '#default_value' => $langs[$key]['pg_account_thousands'],
        '#description'   => t("Please enter thousands delimeter."),
        '#required'      => true,
      );
  
      $form[$key]['pg_account_symbol_position'] = array(
        '#type'          => 'radios',
        '#title'         => t('Simbol position'),
        '#options'       => array ( 1 => t('Left'), 0 => t('Right')),
        '#default_value' => $langs[$key]['pg_account_symbol_position'],
        '#description'   => t("Please select symbol position."),
        '#required'      => true,
      );
    }
  }else{
    $form['pg_account_rate'] = array(
      '#type'          => 'textfield',
      '#title'         => t('Rate to site currency'),
      '#default_value' => variable_get('pg_account_rate','1.00'),
      '#description'   => t("Please enter Account Balance rate according to site currency."),
      '#required'      => true,
    );
    $form['pg_account_symbol'] = array(
      '#type'          => 'textfield',
      '#title'         => t('Symbol'),
      '#default_value' => variable_get('pg_account_symbol','$'),
      '#description'   => t("Please enter symbol of the Account Balance currency."),
      '#required'      => true,
    );
    
  
    $form['pg_account_decimal_places'] = array(
      '#type'          => 'textfield',
      '#title'         => t('Decimal places'),
      '#default_value' => variable_get('pg_account_decimal_places',2),
      '#description'   => t("Please enter number of decimal places."),
      '#required'      => true,
    );

    $form['pg_account_decimal'] = array(
      '#type'          => 'textfield',
      '#title'         => t('Decimal delimiter'),
      '#default_value' => variable_get('pg_account_decimal','.'),
      '#description'   => t("Please enter decimal delimiter."),
      '#required'      => true,
    );

    $form['pg_account_thousands'] = array(
      '#type'          => 'textfield',
      '#title'         => t('Thousands places'),
      '#default_value' => variable_get('pg_account_thousands',','),
      '#description'   => t("Please enter thousands delimeter."),
      '#required'      => true,
    );

    $form['pg_account_symbol_position'] = array(
      '#type'          => 'radios',
      '#title'         => t('Simbol position'),
      '#options'       => array ( 1 => t('Left'), 0 => t('Right')),
      '#default_value' => variable_get('pg_account_symbol_position',1),
      '#description'   => t("Please select symbol position."),
      '#required'      => true,
    );  
  }

  $form['submit'] = array(
    '#type'  => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}


function pg_account_settings_validate($form, &$form_state) {
  $count=1;
  if(module_exists('locale') ){
    $lang_list = language_list('enabled');
    $count = count($lang_list[1]);
  }

  if($count > 1 ){
    foreach($lang_list[1] as $key => $lang){
      if($form_state['values'][$key]['pg_account_rate']<=0){
        form_set_error($key.'][pg_account_rate', t('Rate must be more than 0.', array('%rate' => $form_state['values'][$key]['pg_account_rate'])));
      }  
    }
    
  }else{
    if($form_state['values']['pg_account_rate']<=0){
      form_set_error('pg_account_rate', t('Rate must be more than 0.', array('%rate' => $form_state['values']['pg_account_rate'])));
    }
  }
}

function pg_account_settings_submit($form, &$form_state){
  $count=1;
  if(module_exists('locale') ){
    $lang_list = language_list('enabled');
    $count = count($lang_list[1]);
  }

  if($count > 1 ){
    
    $langs=variable_get('pg_account_rates',array());
    $default = language_default();
    
    foreach($lang_list[1] as $key => $lang){
      if($default->language  == $key ){
        variable_set('pg_account_rate',$form_state['values'][$key]['pg_account_rate']);
        variable_set('pg_account_symbol',$form_state['values'][$key]['pg_account_symbol']);
        variable_set('pg_account_decimal_places',$form_state['values'][$key]['pg_account_decimal_places']);
        variable_set('pg_account_decimal',$form_state['values'][$key]['pg_account_decimal']);
        variable_set('pg_account_thousands',$form_state['values'][$key]['pg_account_thousands']);
        variable_set('pg_account_symbol_position',$form_state['values'][$key]['pg_account_symbol_position']);
      
      }
      $langs[$key]=$form_state['values'][$key];
    }
    variable_set('pg_account_rates',$langs);
    
  }else{
    if(isset($form_state['values']['pg_account_rate']))
      variable_set('pg_account_rate',$form_state['values']['pg_account_rate']);
    if(isset($form_state['values']['pg_account_symbol']))
      variable_set('pg_account_symbol',$form_state['values']['pg_account_symbol']);
  
    if(isset($form_state['values']['pg_account_decimal_places']))
      variable_set('pg_account_decimal_places',$form_state['values']['pg_account_decimal_places']);
  
    if(isset($form_state['values']['pg_account_decimal']))
      variable_set('pg_account_decimal',$form_state['values']['pg_account_decimal']);
  
    if(isset($form_state['values']['pg_account_thousands']))
      variable_set('pg_account_thousands',$form_state['values']['pg_account_thousands']);
  
    if(isset($form_state['values']['pg_account_symbol_position']))
      variable_set('pg_account_symbol_position',$form_state['values']['pg_account_symbol_position']);
  }
    
}

function pg_account_refund($form_state,$t){
  $form['txnid'] = array(
    '#type'        => 'value',
    '#value'       => $t->txnid,
  );
  
  $account=user_load($t->uid);

  if($t->amount < 0 ){
    if($t->pgapi_txnid){
      //payment
      $txn=pgapi_transaction_load($t->pgapi_txnid);
      $description=$txn->description;
    }else{
      //withdraw
      $description=t('Withdraw via %paymentsystem ', array('%paymentsystem' => module_invoke($t->method, 'pgaccount_withdraw','display name')));
    }
  }else{
    if($t->pgapi_txnid){
      //add funds
      $txn=pgapi_transaction_load($t->pgapi_txnid);
      $description=$txn->description ? $txn->description  : $txn->title;
    }else{
      //withdraw
      $description=$t->description;
    }
  }

  if($t->method == 'manual'){
    $description=$txn->description;
  }
     
  $rows=array(
    array(t('Transaction ID'),$t->txnid),
    array(t('User'),theme('username',$account)),
    array(t('Amount'),pgapi_format_price('pg_account',abs($t->amount),pg_account_getCurrentSymbol($account))),
    array(t('Date'),format_date($txn->changed,'small')),
    array(t('Description'),$description),
    array(t('Payment status'),pgapi_get_status($t->status)),
    array(t('Workflow status'),pgapi_get_workflow($t->workflow))
  );
  
  $form['transaction_info'] = array(
    '#type'        => 'markup',
    '#value'       => theme('table',array(),$rows),
  );
  
  $form['submit'] = array(
    '#type'  => 'submit',
    '#value' => t('Refund money'),
  );

  return $form;
}

function pg_account_refund_submit($form, &$form_state){
  $txnid=$form_state['values']['txnid'];

  $t=pg_account_transaction_load($txnid);
  $account=user_load($t->uid);
  
  $t->status=PG_REFUNDED;
  $t->workflow=PG_WORKFLOW_CANCELED;
  $t->changed=time();
  pg_account_transaction_save($t);
  
  $b=pg_account_balance_load($t->uid);
  $amount = abs($t->amount);
    
  if($b->norecord){
    $b->balance = $amount;
    $b->uid=$t->uid;
    drupal_write_record('pg_account_balance', $b);
  }else{
    $b->balance = $b->balance + $amount;
    drupal_write_record('pg_account_balance', $b, 'uid');
  }

  drupal_set_message(t('You refunded !formated_amount to !user',array('!user' => theme('username',$account), '!formated_amount'=>pgapi_format_price('pg_account', abs($t->amount) ,pg_account_getCurrentSymbol($account) ))));
}


function pg_account_cancel($form_state,$t){
  $form['txnid'] = array(
    '#type'        => 'value',
    '#value'       => $t->txnid,
  );
  $account=user_load($t->uid);
 
  if($t->amount < 0 ){
    if($t->pgapi_txnid){
      //payment
      $txn=pgapi_transaction_load($t->pgapi_txnid);
      $description=$txn->description;
    }else{
      //withdraw
      $description=t('Withdraw via %paymentsystem ', array('%paymentsystem' => module_invoke($t->method, 'pgaccount_withdraw','display name')));
    }
  }else{
    if($t->pgapi_txnid){
      //add funds
      $txn=pgapi_transaction_load($t->pgapi_txnid);
      $description=$txn->description ? $txn->description  : $txn->title;
    }else{
      //withdraw
      $description=$t->description;
    }
  }

  if($t->method == 'manual'){
    $description=$t->description;
  }
     
  $rows=array(
    array(t('Transaction ID'),$t->txnid),
    array(t('User'),theme('username',$account)),
    array(t('Amount'),pgapi_format_price('pg_account',abs($t->amount),pg_account_getCurrentSymbol($account))),
    array(t('Date'),format_date($txn->changed,'small')),
    array(t('Description'),$description),
    array(t('Payment status'),pgapi_get_status($t->status)),
    array(t('Workflow status'),pgapi_get_workflow($t->workflow))
  );
  
  $form['transaction_info'] = array(
    '#type'        => 'markup',
    '#value'       => theme('table',array(),$rows),
  );
  $form['submit'] = array(
    '#type'  => 'submit',
    '#value' => t('Cancel transaction'),
  );

  return $form;
}

function pg_account_cancel_submit($form, &$form_state){
  $txnid=$form_state['values']['txnid'];

  $t=pg_account_transaction_load($txnid);
  $account=user_load($t->uid);
  
  $t->status=PG_CANCELED;
  $t->workflow=PG_WORKFLOW_CANCELED;
  $t->changed=time();
  pg_account_transaction_save($t);
  
  $b=pg_account_balance_load($t->uid);
  $amount = $t->amount;
    
  if($b->norecord){
    $b->balance = $amount;
    $b->uid=$t->uid;
    drupal_write_record('pg_account_balance', $b);
  }else{
    $b->balance = $b->balance - $amount;
    drupal_write_record('pg_account_balance', $b, 'uid');
  }

  drupal_set_message(t('You canceled transaction !txnid for !formated_amount to !user',array('!txnid' => $t->txnid, '!user' => theme('username',$account), '!formated_amount'=>pgapi_format_price('pg_account', abs($t->amount) ,pg_account_getCurrentSymbol($account) ))));
}



function pg_account_send($form_state,$t){
  $form['txnid'] = array(
    '#type'        => 'value',
    '#value'       => $t->txnid,
  );
  
  $account=user_load($t->uid);
 
  if($t->amount < 0 ){
    if($t->pgapi_txnid){
      //payment
      $txn=pgapi_transaction_load($t->pgapi_txnid);
      $description=$txn->description;
    }else{
      //withdraw
      $description=t('Withdraw via %paymentsystem ', array('%paymentsystem' => module_invoke($t->method, 'pgaccount_withdraw','display name')));
    }
  }else{
    if($t->pgapi_txnid){
      //add funds
      $txn=pgapi_transaction_load($t->pgapi_txnid);
      $description=$txn->description ? $txn->description  : $txn->title;
    }else{
      //withdraw
      $description=$t->description;
    }
  }

  if($t->method == 'manual'){
    $description=$txn->description;
  }
     
  $rows=array(
    array(t('Transaction ID'),$t->txnid),
    array(t('User'),theme('username',$account)),
    array(t('Amount'),pgapi_format_price('pg_account',abs($t->amount),pg_account_getCurrentSymbol($account))),
    array(t('Date'),format_date($txn->changed,'small')),
    array(t('Description'),$description),
    array(t('Payment status'),pgapi_get_status($t->status)),
    array(t('Workflow status'),pgapi_get_workflow($t->workflow))
  );
  
  $form['transaction_info'] = array(
    '#type'        => 'markup',
    '#value'       => theme('table',array(),$rows),
  );

  if ($module_name= module_invoke($t->method, 'pgaccount_withdraw','display name')) {
    $form['details']=array(
      '#type' => 'fieldset',
      '#title' => t('Transfer to !title' , array ('!title' => $module_name)),
    );
    $form['details'] += module_invoke($t->method, 'pgaccount_withdraw','operator form',$t);
  }

  $form['sent'] = array(
    '#type'  => 'submit',
    '#value' => t('Money sent'),
  );
  $form['goback'] = array(
    '#type'  => 'submit',
    '#value' => t('Go back'),
  );

  return $form;
}

function pg_account_send_submit($form, &$form_state){
  if($form_state['values']['op']  == $form_state['values']['sent']){
    $txnid=$form_state['values']['txnid'];
    $t=pg_account_transaction_load($txnid);
    $account = user_load($t->uid);
    $module_name= module_invoke($t->method, 'pgaccount_withdraw','display name');
    $t->status=PG_COMPLETED;
    $t->workflow=PG_WORKFLOW_COMPLETED;
    $t->changed=time();
    pg_account_transaction_save($t);
    drupal_set_message(t('You transferred !formated_amount to !method for !user',array('!user' => theme('username',$account), '!formated_amount'=>pgapi_format_price('pg_account', abs($t->amount) ,pg_account_getCurrentSymbol($account) ), '!method' => $module_name)));
  }
}

function pgaccount_common(){
  $form['#tree'] = TRUE;
  $pgaccount_withdraw=variable_get('pgaccount_withdraw',array());
  foreach (module_implements('pgaccount_withdraw') as $module) {
    $default='';
    if ($module_name= module_invoke($module, 'pgaccount_withdraw','display name')) {
      $form[$module]['modulename']=array ('#value' => $module_name);
      if($pgaccount_withdraw[$module]){
        $default='checked';
      }
      $form[$module]['checkbox']=array(
        '#type' => 'checkbox',
        '#default_value' => $default
      );
    }
  }


  $form['submit'] = array(
    '#type'  => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}


function pgaccount_common_submit($form, &$form_state){
  $pgaccount_withdraw=array();
  foreach($form_state['values'] as $key => $val) {
    if($val['checkbox']==1){
      $pgaccount_withdraw[$key]=true;
    }
  }
  variable_set('pgaccount_withdraw',$pgaccount_withdraw);
  drupal_set_message(t('Withdraw settings saved.'));
}


function pg_account_moneyout_setting(){
  $res=array();
  foreach (module_implements('pgaccount_withdraw') as $module) {
    if ($datas= module_invoke($module, 'pgaccount_withdraw','data')) {
      foreach($datas as $key => $mdata){
        $res[$mdata['title']]=$mdata['values'];
      }//foreach
    }//if
  }//foreach
  return theme('pg_account_moneyout_setting',$res);
}

function pg_account_account($account){

  $tabs=array();
  foreach (module_implements('accountTabs') as $module) {
    if (function_exists($function = $module . '_accountTabs')) {
      $module_info = $function('info',$account);
    }
    if(is_array($module_info)){
      foreach($module_info as $key => $submod_info){
        $function=$module.'_accountTabs';
        $submod_info['form'] = $function('get form', $key, $account);
        $submod_info['key'] = $key;
        $submod_info['module'] = $module;
        $tabs[]=$submod_info;
      }

    }else{
      $key=0;
      $function=$module.'_accountTabs';
      $module_info['form'] = $function('get form', $key, $account);
      $module_info['key'] = 0;
      $module_info['module'] = $module;
      $tabs[]=$module_info;
    }
  }
  
  uasort($tabs,'_user_sort');
  
  $balance=pg_account_balance_load($account->uid);
  
  $data=array(
    'user' => $account,
    'balance' => pgapi_format_price('pg_account',$balance->balance,pg_account_getCurrentSymbol($account)),
    'tabs' => $tabs,
    'transactions' => pg_account_personal_history($account),
  );
  
  return theme('pg_account_account',$data);
}


function pg_account_operator(){
  
  
  $tabs=array();
  foreach (module_implements('operatorTabs') as $module) {
    if ($module_info= module_invoke($module, 'operatorTabs','info')) {
      if(is_array($module_info)){
        foreach($module_info as $key => $submod_info){
          $submod_info['form'] = module_invoke($module, 'operatorTabs','get form', $key);
          $submod_info['key'] = $key;
          $submod_info['module'] = $module;
          $tabs[]=$submod_info;
        }

      }else{
        $module_info['form'] = module_invoke($module, 'operatorTabs','get form',0);
        $module_info['key'] = 0;
        $module_info['module'] = $module;
        $tabs[]=$module_info;
      }
    }
  }
  
  uasort($tabs,'_user_sort');
  
  $data=array(
    'tabs' => $tabs,
    'transactions' => pg_account_history(),
  );
  
  return theme('pg_account_operator',$data);
}



/* JS callbacks */


function pg_account_wrapper_js(){

  $method=$_POST['method'];

  
  $cached_form_state = array();
  // Load the form from the Form API cache.
  if (!($cached_form = form_get_cache($_POST['form_build_id'], $cached_form_state)) || !isset($cached_form['method_details'])) {
    form_set_error('form_token', t('Validation error, please try again. If this error persists, please contact the site administrator.'));
    $output = theme('status_messages');
    print drupal_to_js(array('status' => TRUE, 'data' => $output));
    exit();
  }
  
  $form_state = array('values' => $_POST);
  
  $form = module_invoke($method, 'pgaccount_withdraw','get form', $form_state);
  
  
  //delete previos elements
  $allow=array('#tree','#prefix','#suffix');
  foreach($cached_form['method_details'] as $key => $val){
    if(!in_array($key,$allow)){
      unset($cached_form['method_details'][$key]);
    }
  }
  
  $cached_form['method_details'] = array_merge($cached_form['method_details'], $form);
  
  form_set_cache($_POST['form_build_id'], $cached_form, $cached_form_state);
  
  $form += array(
    '#post' => $_POST,
    '#programmed' => FALSE,
    '#tree' => TRUE,
    '#parents' => array('method_details'),
  );

  $form_state = array('submitted' => FALSE);
  $form = form_builder('pgapi_extra_form', $form, $form_state);
  $javascript=drupal_add_js(NULL, NULL, 'header');
  if(is_array($javascript['setting'])){
    $embed_prefix = "\n<!--//--><![CDATA[//><!--\n";
    $embed_suffix = "\n//--><!]]>\n";    
    $setting ='<script type="text/javascript">' . $embed_prefix . 'jQuery.extend(Drupal.settings, '. drupal_to_js(call_user_func_array('array_merge_recursive', $javascript['setting'])) .");". $embed_suffix ."</script>\n";
  }

  $output .= $setting.$scripts.theme('status_messages') . drupal_render($form);

  print drupal_to_js(array('status' => TRUE, 'data' => $output));

  exit;
}
