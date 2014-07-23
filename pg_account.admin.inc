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
