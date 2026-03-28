
// [10/31/2006 by novice] 
// √‚√≥ : http://www.flipcode.org/cgi-bin/fcarticles.cgi?show=63806


/*
//---------------------------------------------------------------------------------------------
// Example code
//---------------------------------------------------------------------------------------------

#pragma TODO(  "We have still to do some work here..." )
#pragma FIXME( "Limits are not controlled in that function or things like that" ) 
#pragma todo(  "Have a look to flipcode daily !" ) 
#pragma todo(  "Sleep..." )  
#pragma fixme( "It seems that there is some leaks in that object" ) 
#pragma FILE_LINE    
#pragma NOTE( " \n\
A free format multiline, comment............\n\
So I can put a whole text here              \n\
-------------------------------------------------")

*/

//---------------------------------------------------------------------------------------------
// FIXMEs / TODOs / NOTE macros
//---------------------------------------------------------------------------------------------
#define _QUOTE(x) # x
#define QUOTE(x) _QUOTE(x)
#define __FILE__LINE__ __FILE__ "(" QUOTE(__LINE__) ") : "
#define NOTE( x )  message( x )
#define FILE_LINE  message( __FILE__LINE__ )
#define TODO( x )  message( __FILE__LINE__"\n"           \
	" ------------------------------------------------\n" \
	"|  TODO :   " x "\n" \
	" -------------------------------------------------\n" )
#define FIXME( x )  message(  __FILE__LINE__"\n"           \
	" ------------------------------------------------\n" \
	"|  FIXME :  " x "\n" \
	" -------------------------------------------------\n" )
#define todo( x )  message( __FILE__LINE__" TODO :   " #x "\n" ) 
#define fixme( x )  message( __FILE__LINE__" FIXME:   " #x "\n" ) 

