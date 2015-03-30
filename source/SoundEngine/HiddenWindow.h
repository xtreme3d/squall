#ifndef _HIDDEN_WINDOW_H_INCLUDED_
#define _HIDDEN_WINDOW_H_INCLUDED_

#include <windows.h>

HWND CreateHiddenWindow(void);  				   // создание скрытого окна
void ReleaseHiddenWindow(HWND window);  		   // удаление скрытого окна

#endif
