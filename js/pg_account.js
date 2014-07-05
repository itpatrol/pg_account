/* Global Vars*/
var clickAssigned;

function createCookie(name,value,days) {
	if (days) {
		var date = new Date();
		date.setTime(date.getTime()+(days*24*60*60*1000));
		var expires = "; expires="+date.toGMTString();
	}
	else var expires = "";
	document.cookie = name+"="+value+expires+"; path=/";
}

function readCookie(name) {
	var nameEQ = name + "=";
	var ca = document.cookie.split(';');
	for(var i=0;i < ca.length;i++) {
		var c = ca[i];
		while (c.charAt(0)==' ') c = c.substring(1,c.length);
		if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length,c.length);
	}
	return null;
}

function eraseCookie(name) {
	createCookie(name,"",-1);
}


Drupal.behaviors.PGAccount = function (context) {
  if(!clickAssigned){
    var curTab=readCookie('pg_account_tabname');
    if(curTab){
      $('div.' + (curTab)).slideDown();
      $("#accountTools ul.actions a[rel=" + curTab + "]").parent().addClass('selected');
    }
    $("#accountTools ul.actions a").click(function(){
    
      //protect from double call
      clickAssigned = true;

      //if selected, hide then
      var selected=false;
      if( $(this).parents().hasClass('selected') ){
        selected = true;
        //hide form
        $('div.' + $(this).attr('rel')).slideUp();
        
        eraseCookie('pg_account_tabname');
        
      }else{
        
        //hide other forms, show mine
        $("#accountForms div.wrapper").each(function(){
          if($(this).is(':visible')){
            $(this).slideUp();
          }
        });
        
        //show form
        $('div.' + $(this).attr('rel')).slideDown();
        createCookie('pg_account_tabname', $(this).attr('rel'),1);
      }
      
      //remove selected from others
      $("#accountTools ul.actions li").each(function(){
        $(this).removeClass('selected');
      });
      
      //add selected if isnot selected
      if( !selected ){
        $(this).parent().addClass('selected');
      }
      return false;
    });
  }
};