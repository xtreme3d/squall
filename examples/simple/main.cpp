#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include "Squall.h"

#define K_ESCAPE 27

int main(int argc, const char** argv)
{
    int sample, channel, key;

    if (SQUALL_Init(0) < 0)
    {
        SQUALL_Free();
        return 0;
    }

    sample = SQUALL_Sample_LoadFile("test.mp3", 1, 0);
    channel = SQUALL_Sample_Play(sample, 0, 0, 1);
    
    printf("Press Escape to exit\n");
    
    do
    {
        if (kbhit())
            key = getch();
        Sleep(100);
    }
    while (key != K_ESCAPE);

    SQUALL_Free();
    return 0;
}
