#ifndef ASCIILOGO_H
#define ASCIILOGO_H

#include <stdio.h> /*printf */

static const char *ascii_logo =
"	||   //								\n"
"	||  //								\n"
"	|| //								\n"
"	||//								\n"
"	||\\\\								\n"	
"	|| \\\\								\n"
"	||  \\\\							\n"
"	||   \\\\							\n"
;

static void logo(void){
	printf("%s", ascii_logo);
}

#endif // ASCIILOGO_H
