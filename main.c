#include "windows.h"
#include "time.h"
#include "stdio.h"

unsigned long long int ppuCycles=0;
unsigned long long int cycleCounter=0;
unsigned char OAM[256],OAMADDR=0;
unsigned char memPPU[65536];
char vblank=0;
double seconds=100;
clock_t t;

#include "cpu.h"
#include "ppu.h"

int main()
{
    double time_taken;
    char* filename="nestest.nes";
    loadROM(filename);
    system("pause");
    t = clock();
    HANDLE threadovi[2];
    threadovi[0] = CreateThread(NULL,2097152,mainPPU,NULL,0,0);
    threadovi[1] = CreateThread(NULL,2097152,mainCPU,NULL,0,0);
    WaitForMultipleObjects(2,threadovi,1,INFINITE);
    t = clock() - t;

    time_taken += ((double)t)/CLOCKS_PER_SEC;
    printf("TIME: %f seconds\n",time_taken);
    return 0;
}
