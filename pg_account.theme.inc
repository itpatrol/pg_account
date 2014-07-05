<?php

/********************************************************************
 * Functions theme
 ********************************************************************/

function theme_pg_account_fail($t){
  $header = array(
    array( 'data' => $t->title, 'colspan' => '2'),
  );

  $rows=array();

  $account = user_load($t->uid);
  $row=array();
  $row[]= t('Price');
  $row[]= t('!formated_amount', array('!formated_amount' => pgapi_format_price('pg_account',$t->amount,pg_account_getCurrentSymbol($account) )));
  $rows[] = $row;

  $row=array();
  $row[]= t('Status');
  $row[]= pgapi_get_status($t->status);
  $rows[] = $row;

  $row=array();
  $row[]= t('By method');
  $row[]= module_invoke($t->method, 'pgapi_gw','display name');
  $rows[] = $row;

  $row=array();
  $row[]= t('Reason');
  $row[]= pgapi_get_status($t->description);
  $rows[] = $row;

  $output = theme('table', $header, $rows);
  return $output;
}

function theme_pg_account_pending($t){
  $header = array(
    array( 'data' => $t->title, 'colspan' => '2'),
  );

  $rows=array();

  $account = user_load($t->uid);
  $row=array();
  $row[]= t('Price');
  $row[]= t('!formated_amount', array('!formated_amount' => pgapi_format_price('pg_account',$t->amount,pg_account_getCurrentSymbol($account) )));
  $rows[] = $row;

  $row=array();
  $row[]= t('Status');
  $row[]= pgapi_get_status($t->status);
  $rows[] = $row;

  $row=array();
  $row[]= t('By method');
  $row[]= module_invoke($t->method, 'pgapi_gw','display name');
  $rows[] = $row;

  $row=array();
  $row[]= t('Reason');
  $row[]= pgapi_get_status($t->description);
  $rows[] = $row;

  $output = theme('table', $header, $rows);
  return $output;
}

function theme_pg_account_complete($t){
  $header = array(
    array( 'data' => $t->title, 'colspan' => '2'),
  );

  $rows=array();

  $account = user_load($t->uid);
  $row=array();
  $row[]= t('Price');
  $row[]= t('!formated_amount', array('!formated_amount' => pgapi_format_price('pg_account',$t->amount,pg_account_getCurrentSymbol($account) )));
  $rows[] = $row;

  $row=array();
  $row[]= t('Status');
  $row[]= pgapi_get_status($t->status);
  $rows[] = $row;

  $row=array();
  $row[]= t('By method');
  $row[]= module_invoke($t->method, 'pgapi_gw','display name');
  $rows[] = $row;

  $row=array();
  $row[]= t('New balance');
  $b=pg_account_balance_load($t->uid);
  $row[]= t('Your current balance is !formated_balance', array('!formated_balance' => pgapi_format_price('pg_account',$b->balance ,pg_account_getCurrentSymbol($account))));
  $rows[] = $row;
  
  $output = theme('table', $header, $rows);
  return $output;
}


function theme_pg_account_filters($form) {
  drupal_add_css(drupal_get_path('module', 'pg_account') .'/css/pg_account.css');
  if (sizeof($form['current'])) {
    $output.= '<div>';
  	$output.= t('Active filters').':';
    $output.= '<ul class="clear-block">';
    foreach (element_children($form['current']) as $key) {
      $output.= '<li>'. drupal_render($form['current'][$key]) .'</li>';
    }
    $output.= '</ul>';
    $output.= '</div>';
  }

  if(is_array($form['filter']) && is_array($form['status'])){
    $output .= '<dl class="multiselect" >'. (sizeof($form['current']) ? '<dt><em>'. t('and') .'</em> '. t('where') .'</dt>' : '') .'<dd class="a" >';
    foreach (element_children($form['filter']) as $key) {
      $output .= drupal_render($form['filter'][$key]);
    }
    $output .= '</dd>';
  
    $output .= '<dt>'. t('is') .'</dt><dd class="b">';
    foreach (element_children($form['status']) as $key) {
      $output .= drupal_render($form['status'][$key]);
    }
    $output .= '</dd>';
    $output .= '</dl>';
  
  }

  $output .= '<div style="clear:both;" class="container-inline" id="user-admin-buttons">'. drupal_render($form['buttons']) .'</div><div style="clear:both;" ></div>';

  return $output;
}

function theme_pg_account_account($data){
  
  drupal_add_css(drupal_get_path('module', 'pg_account') .'/css/pg_account.css');
  drupal_add_js(drupal_get_path('module', 'pg_account') .'/js/pg_account.js');

  $tabs ='';
  $forms='';
  foreach($data['tabs'] as $tab){
    $name= $tab['module'].'-'.$tab['key'];
    $tabs .= '<li><a href="#" class="'.$name.'" rel="'.$name.'-form">'.$tab['title'].'</a></li>';
    $forms .= '<div class="'.$name.'-form wrapper" id="wrapper-'.$name.'">'.$tab['form'].'</div>';
  }

  
  $output ='
  <div id="accountTools">
    <ul class="actions">
      <li><div class="balance">'.t('Balance !balance', array('!balance' => $data['balance'])).'</div></li>
      '.$tabs.'
     </ul>
    <div class="clear"></div>
  </div>
  <div id="accountForms">
    '.$forms.' 
  </div>
  <div id="accountTransactions">'.$data['transactions'].'</div>
  
  ';
  return $output;
}

function theme_pg_account_operator($data){
  
  drupal_add_css(drupal_get_path('module', 'pg_account') .'/css/pg_account.css');
  drupal_add_js(drupal_get_path('module', 'pg_account') .'/js/pg_account.js');

  $tabs ='';
  $forms='';
  foreach($data['tabs'] as $tab){
    $name= $tab['module'].'-'.$tab['key'];
    $tabs .= '<li><a href="#" class="'.$name.'" rel="'.$name.'-form">'.$tab['title'].'</a></li>';
    $forms .= '<div class="'.$name.'-form wrapper" id="wrapper-'.$name.'">'.$tab['form'].'</div>';
  }

  
  $output ='
  <div id="accountTools">
    <ul class="actions">
      '.$tabs.'
     </ul>
    <div class="clear"></div>
  </div>
  <div id="accountForms">
    '.$forms.' 
  </div>
  <div id="accountTransactions">'.$data['transactions'].'</div>
  
  ';
  return $output;
}