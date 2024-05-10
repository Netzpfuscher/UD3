#include "USBMIDI_1.h"
#include "console.h"

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
 
int _kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;
 
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
	
  ch = getchar();
 
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
 
  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }
 
  return 0;
}

uint8_t USBMIDI_1_initVar=0;

uint8_t USBMIDI_1_IsConfigurationChanged(){
	return 0;
}

uint8_t USBMIDI_1_GetConfiguration(){
	return 1;
}

void USBMIDI_1_PutData(uint8_t * buffer, uint16_t count){
	console_print("%.*s",count,buffer);

}

uint8_t c;
uint8_t USBMIDI_1_CDCIsReady(){
	return 1;
}

uint8_t USBMIDI_1_DataIsReady(){
	if (_kbhit()){
		c= getchar();

		return 1;
	}
	return 0;
}

uint16_t USBMIDI_1_GetAll(uint8_t * buffer){
	if (c == '\n') {
		// TTerm treats \n as backspace. Since none of the "usual" terminals send it this does not matter outside the
		// simulator, and in the sim we just special-case it to send the "correct" \r instead
		*buffer = '\r';
	} else {
		*buffer = c;
	}
	return 1;
}
	
