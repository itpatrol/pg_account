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


function theme_user_balance($vars){
  $balance = pgapi_format_price($vars['balance']->balance);
  $output = '<div class="balance"> '. t('Balance') . $balance . '</div>'; 
  return $output;
}
