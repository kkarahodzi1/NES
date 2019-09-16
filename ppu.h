#include "stdio.h"
#include "windows.h"

unsigned char demoScreen[240][256];


unsigned char OAMSelect[8][4];
unsigned short int twoTilesHi,twoTilesLo,palettesHi,palettesLo;
unsigned char bitmapDataSPR[8][2],counters[8],latchesPalette[8];
unsigned char priority;

void exampleScreen()
{
    int i,j;
    COLORREF color[64] = {RGB(124,124,124),RGB(252,0,0),RGB(188,0,0),RGB(188,40,68),RGB(132,0,148),RGB(32,0,168),RGB(0,16,168),RGB(0,20,136),
                          RGB(0,48,80),RGB(0,120,0),RGB(0,104,0),RGB(0,88,0),RGB(88,64,0),RGB(0,0,0),RGB(0,0,0),RGB(0,0,0),
                          RGB(188,188,188),RGB(248,120,0),RGB(248,88,0),RGB(252,68,104),RGB(204,0,216),RGB(88,0,228),RGB(0,56,248),RGB(16,92,228),
                          RGB(0,124,172),RGB(0,184,0),RGB(0,168,0),RGB(68,168,0),RGB(136,136,0),RGB(0,0,0),RGB(0,0,0),RGB(0,0,0),
                          RGB(248,248,248),RGB(252,188,60),RGB(252,136,104),RGB(248,120,152),RGB(248,120,248),RGB(152,88,248),RGB(88,120,248),RGB(68,160,252),
                          RGB(0,184,248),RGB(24,248,184),RGB(84,216,88),RGB(152,248,88),RGB(216,232,0),RGB(120,120,120),RGB(0,0,0),RGB(0,0,0),
                          RGB(252,252,252),RGB(252,228,164),RGB(248,184,184),RGB(248,184,216),RGB(248,184,248),RGB(192,164,248),RGB(176,208,240),RGB(168,224,252),
                          RGB(120,216,248),RGB(120,248,216),RGB(184,248,184),RGB(216,248,184),RGB(252,252,0),RGB(248,216,248),RGB(0,0,0),RGB(0,0,0)};
    SetConsoleTitle("Pixel In Console?"); // Set text of the console so you can find the window
    COLORREF *arr = (COLORREF*) calloc(2*240*2*256, sizeof(COLORREF));

    HWND hwnd = FindWindow(NULL, "Pixel In Console?"); // Get the HWND
    HDC hdc = GetDC(hwnd); // Get the DC from that HWND
    for( i = 0 ; i < 240 ; i++ )
    {
        for( j = 0 ; j < 256 ;j++ )
        {
            arr[(2*i)*2*256+2*j]=color[demoScreen[i][j]];
            arr[(2*i+1)*2*256+2*j]=color[demoScreen[i][j]];
            arr[(2*i)*2*256+2*j+1]=color[demoScreen[i][j]];
            arr[(2*i+1)*2*256+2*j+1]=color[demoScreen[i][j]];
            //SetPixel(hdc, j, i, color[demoScreen[i][j]]); // SetPixel(HDC hdc, int x, int y, COLORREF color)
        }
    }
    HBITMAP map = CreateBitmap(2*256, // width. 512 in my case
                               2*240, // height
                               1, // Color Planes, unfortanutelly don't know what is it actually. Let it be 1
                               8*4, // Size of memory for one pixel in bits (in win32 4 bytes = 4*8 bits)
                               (void*) arr);
    HDC src = CreateCompatibleDC(hdc); // hdc - Device context for window, I've got earlier with GetDC(hWnd) or GetDC(NULL);
    SelectObject(src, map); // Inserting picture into our temp HDC
    // Copy image from temp HDC to window
    BitBlt(hdc, // Destination
           0,  // x and
           0,  // y - upper-left corner of place, where we'd like to copy
           2*256, // width of the region
           2*240, // height
           src, // source
           0,   // x and
           0,   // y of upper left corner  of part of the source, from where we'd like to copy
           SRCCOPY);

    DeleteDC(src);
    free(arr);
    DeleteObject(map);
    ReleaseDC(hwnd, hdc); // Release the DC
    DeleteDC(hdc); // Delete the DC

}

void waitForCPU()
{
    while(ppuCycles>cycleCounter*3 && ppuCycles<5360520*seconds);
}


unsigned char getBit(unsigned char bite, unsigned char bit)
{
    return (bite&(1<<bit))>>bit;
}

unsigned char getPixelSPR()
{
    unsigned char render=0;
    int i;
    for(i=0;i<8;i++)
    {
        if(counters[i])//decrement until zero
            counters[i]--;
        else //if zero then its active
        {
            //if nothing rendered and pixel is non-transparent(highest bitmap byte are non zero)
            if(!render && ((bitmapDataSPR[i][0]&0xF0) || (bitmapDataSPR[i][1]&0xF0)))
            {
                render=((latchesPalette[i]&0x3)<<2) | ((bitmapDataSPR[i][1]&0x80)>>6) | ((bitmapDataSPR[i][0]&0x80)>>7);
                priority=~(latchesPalette[i]&0x20)>>5;
            }
            bitmapDataSPR[i][0]<<=1;
            bitmapDataSPR[i][1]<<=1;
        }


    }
    return render;
}

unsigned char getPixelBG()
{
    unsigned char render=((palettesHi&0x8000)>>12) | ((palettesLo&0x8000)>>13) | ((twoTilesHi&0x8000)>>14) | ((twoTilesLo&0x8000)>>15);
    twoTilesHi<<=1;
    twoTilesLo<<=1;
    palettesHi<<=1;
    palettesLo<<=1;
    return render;
}

unsigned char getPixel()
{
    unsigned char backGround=getPixelBG(),
                  sprite=getPixelSPR();
    unsigned short int pixel=0x3F00;
    if((priority && sprite%4) || !(backGround%4))
    {
        if(!(sprite%4))
            pixel=0x3F00;
        else
            pixel=0x3F10+sprite;
    }
    else
        pixel=0x3F00+backGround;

    if(memCPURead(0x2001)&1)//greyscale mode
        pixel&=0x30;
    return memPPU[pixel];
}

void visibleScanline(unsigned char scanlineNum)
{
    int i,n=0,j=0;
    unsigned short int baseNT= 0x2000+(memCPURead(0x2000)&0x3)*0x400;
    unsigned short int palette=((memCPURead(0x2000)&0x10)>>4)*0x1000;
    unsigned char nameTableEntry,attrTableEntry,tileMapHi,tileMapLo;
    //CYCLE 0 - nothing happens
    ppuCycles+=1;
    //CYCLES 1-256 - process background
    for(i=0;i<256;i++)
    {
        demoScreen[scanlineNum][i] = getPixel();
        waitForCPU();
        ppuCycles+=1;
        if(i%8==0)//get pattern from nametable
        {
            nameTableEntry=memPPU[baseNT+(scanlineNum/8)*32+(i/8)+2];
        }
        else if(i%8==2)//get pallete from atributte table
        {
            attrTableEntry=memPPU[baseNT+0x3C0+(scanlineNum/32)*8+(i+16)/32];
        }
        else if(i%8==4)//get low bits of pattern line
        {
            tileMapLo=memPPU[(((int)nameTableEntry)<<4) | (scanlineNum%8)+palette];
        }
        else if(i%8==6)//get hi bits of pattern line
        {
            tileMapHi=memPPU[(((int)nameTableEntry)<<4) | ((scanlineNum%8)+8)+palette];
        }
        else if(i%8==7)//load registers with new tile data
        {
            twoTilesLo|=tileMapLo;
            twoTilesHi|=tileMapHi;
            if(scanlineNum%32<16)
            {
                if((i+16)%32<16)
                {
                    palettesLo|=0xFF * (attrTableEntry&1);
                    palettesHi|=0xFF * ((attrTableEntry>>1)&1);
                }
                else
                {
                    palettesLo|=0xFF * ((attrTableEntry>>2)&1);
                    palettesHi|=0xFF * ((attrTableEntry>>3)&1);
                }
            }
            else
            {
                if((i+16)%32<16)
                {
                    palettesLo|=0xFF * ((attrTableEntry>>4)&1);
                    palettesHi|=0xFF * ((attrTableEntry>>5)&1);
                }
                else
                {
                    palettesLo|=0xFF * ((attrTableEntry>>6)&1);
                    palettesHi|=0xFF * ((attrTableEntry>>7)&1);
                }
            }
        }

        if(i%2 && i<64)//erase current sprites
        {
            OAMSelect[i/8][(i%8)/2]=0xFF;
        }

        if(i>=64 && n+OAMADDR/4<64)//prepare next sprites
        {
            if(OAM[n*4+OAMADDR]<=scanlineNum && OAM[n*4+OAMADDR]+(memCPURead(0x2000)&(1<<5)?15:7)>=scanlineNum)//is Y in range
            {
                if(j<8)//is OAMSelect full
                {
                        OAMSelect[j][0]=OAM[n*4+OAMADDR];
                        OAMSelect[j][1]=OAM[n*4+OAMADDR+1];
                        OAMSelect[j][2]=OAM[n*4+OAMADDR+2];
                        OAMSelect[j][3]=OAM[n*4+OAMADDR+3];
                        j++;
                }
                else
                {
                    memCPUWrite(0x2002,memCPURead(0x2002)|(1<<5));
                }
            }
            n++;
        }
    }
    //CYCLES 257-320 - process sprites
    for(i=0;i<64;i++)
    {
        ppuCycles+=1;
        waitForCPU();
        OAMADDR=0;
        if(i%8==0)//get garbage from nametable
        {
            nameTableEntry=0xFF;
            if(OAMSelect[i/8][0]==0xff)//no sprite
                latchesPalette[i/8]=0;//load attributes
            else
                latchesPalette[i/8]=OAMSelect[i/8][2];//load attributes
        }
        else if(i%8==2)//get garbage from atributte table
        {
            attrTableEntry=0xFF;
        }
        else if(i%8==4)
        {
            if(memCPURead(0x2000)&(1<<5))//16x8 sprites
            {
                unsigned short int start=((OAMSelect[i/8][1]&1)?0x1000:0x0000)+((OAMSelect[i/8][1]&0xFE)<<5)+(((OAMSelect[i/8][2]&0x40)>>6)^((scanlineNum-OAMSelect[i/8][0])<8)?0x00:0x10);
                unsigned char row=(OAMSelect[i/8][2]&0x80?8-(scanlineNum-OAMSelect[i/8][0]):scanlineNum-OAMSelect[i/8][0])%8;
                if(OAMSelect[i/8][2]&0x40)//flip vertically
                    tileMapLo=(getBit(memPPU[start+row],0)<<7) |
                              (getBit(memPPU[start+row],1)<<6) |
                              (getBit(memPPU[start+row],2)<<5) |
                              (getBit(memPPU[start+row],3)<<4) |
                              (getBit(memPPU[start+row],4)<<3) |
                              (getBit(memPPU[start+row],5)<<2) |
                              (getBit(memPPU[start+row],6)<<1) |
                              (getBit(memPPU[start+row],7));
                else
                    tileMapLo=memPPU[start+row];
            }
            else//8x8 sprites
            {
                unsigned short int start=((memCPURead(0x2000)&(1<<3))?0x1000:0x0000)+(OAMSelect[i/8][1]<<4);
                unsigned char row=(OAMSelect[i/8][2]&0x80?8-(scanlineNum-OAMSelect[i/8][0]):scanlineNum-OAMSelect[i/8][0]);
                if(OAMSelect[i/8][2]&0x40)//flip vertically
                    tileMapLo=(getBit(memPPU[start+row],0)<<7) |
                              (getBit(memPPU[start+row],1)<<6) |
                              (getBit(memPPU[start+row],2)<<5) |
                              (getBit(memPPU[start+row],3)<<4) |
                              (getBit(memPPU[start+row],4)<<3) |
                              (getBit(memPPU[start+row],5)<<2) |
                              (getBit(memPPU[start+row],6)<<1) |
                              (getBit(memPPU[start+row],7));
                else
                    tileMapLo=memPPU[start+row];
            }
            if(OAMSelect[i/8][0]==0xff)
                bitmapDataSPR[i/8][0]=0;
            else
                bitmapDataSPR[i/8][0]=tileMapLo;
        }
        else if(i%8==6)
        {
            if(memCPURead(0x2000)&(1<<5))//16x8 sprites
            {
                unsigned short int start=((OAMSelect[i/8][1]&1)?0x1000:0x0000)+((OAMSelect[i/8][1]&0xFE)<<5)+(((OAMSelect[i/8][2]&0x40)>>6)^((scanlineNum-OAMSelect[i/8][0])<8)?0x00:0x10);
                unsigned char row=(OAMSelect[i/8][2]&0x80?8-(scanlineNum-OAMSelect[i/8][0]):scanlineNum-OAMSelect[i/8][0])%8;
                if(OAMSelect[i/8][2]&0x40)//flip vertically
                    tileMapHi=(getBit(memPPU[start+row+8],0)<<7) |
                              (getBit(memPPU[start+row+8],1)<<6) |
                              (getBit(memPPU[start+row+8],2)<<5) |
                              (getBit(memPPU[start+row+8],3)<<4) |
                              (getBit(memPPU[start+row+8],4)<<3) |
                              (getBit(memPPU[start+row+8],5)<<2) |
                              (getBit(memPPU[start+row+8],6)<<1) |
                              (getBit(memPPU[start+row+8],7));
                else
                    tileMapHi=memPPU[start+row+8];
            }
            else//8x8 sprites
            {
                unsigned short int start=((memCPURead(0x2000)&(1<<3))?0x1000:0x0000)+(OAMSelect[i/8][1]<<4);
                unsigned char row=(OAMSelect[i/8][2]&0x80?8-(scanlineNum-OAMSelect[i/8][0]):scanlineNum-OAMSelect[i/8][0]);
                if(OAMSelect[i/8][2]&0x40)//flip vertically
                    tileMapHi=(getBit(memPPU[start+row+8],0)<<7) |
                              (getBit(memPPU[start+row+8],1)<<6) |
                              (getBit(memPPU[start+row+8],2)<<5) |
                              (getBit(memPPU[start+row+8],3)<<4) |
                              (getBit(memPPU[start+row+8],4)<<3) |
                              (getBit(memPPU[start+row+8],5)<<2) |
                              (getBit(memPPU[start+row+8],6)<<1) |
                              (getBit(memPPU[start+row+8],7));
                else
                    tileMapHi=memPPU[start+row+8];
            }
            if(OAMSelect[i/8][0]==0xff)
                bitmapDataSPR[i/8][1]=0;
            else
                bitmapDataSPR[i/8][1]=tileMapHi;
            counters[i/8]=OAMSelect[i/8][3];
        }
    }

    //CYCLES 321-336 - process next frames scanlines
    scanlineNum++;
    ppuCycles+=8;
    nameTableEntry=memPPU[baseNT+(scanlineNum/8)*32];
    attrTableEntry=memPPU[baseNT+0x3C0+(scanlineNum/32)*8];
    twoTilesLo=memPPU[(((int)nameTableEntry)<<4) | (scanlineNum%8)+palette];
    twoTilesHi=memPPU[(((int)nameTableEntry)<<4) | (scanlineNum%8)+8+palette];

    if(scanlineNum%32<16)
    {
        palettesLo=0xFF * (attrTableEntry&1);
        palettesHi=0xFF * ((attrTableEntry>>1)&1);
    }
    else
    {
        palettesLo=0xFF * ((attrTableEntry>>4)&1);
        palettesHi=0xFF * ((attrTableEntry>>5)&1);
    }
    twoTilesHi<<=8;
    twoTilesLo<<=8;
    palettesHi<<=8;
    palettesLo<<=8;

    ppuCycles+=8;

    nameTableEntry=memPPU[baseNT+(scanlineNum/8)*32+1];
    attrTableEntry=memPPU[baseNT+0x3C0+(scanlineNum/32)*8];

    twoTilesLo|=memPPU[(((int)nameTableEntry)<<4) | (scanlineNum%8)+palette];
    twoTilesHi|=memPPU[(((int)nameTableEntry)<<4) | (scanlineNum%8)+8+palette];

    if(scanlineNum%32<16)
    {
        palettesLo|=0xFF * (attrTableEntry&1);
        palettesHi|=0xFF * ((attrTableEntry>>1)&1);
    }
    else
    {
        palettesLo|=0xFF * ((attrTableEntry>>4)&1);
        palettesHi|=0xFF * ((attrTableEntry>>5)&1);
    }

    //CYCLES 337-340
    for(i=0;i<4;i++)
    {
        ppuCycles+=1;
        waitForCPU();
    }
}

void processFrame()
{
    memCPUWrite(0x2002,memCPURead(0x2002)&~(0x80));//Set vblank flag
    unsigned short int palette=((memCPURead(0x2000)&0x10)>>4)*0x1000;
    int i;
    unsigned short int baseNT= 0x2000+(memCPURead(0x2000)&0x3)*0x400;
    unsigned char nameTableEntry,attrTableEntry,tileMapHi,tileMapLo;

    //prerender scanline - LOAD FIRST TWO TILES -check this, maybe load sprites

    //first tile
    nameTableEntry=memPPU[baseNT];
    attrTableEntry=memPPU[baseNT+0x3C0];
    twoTilesLo=memPPU[(((int)nameTableEntry)<<4) + palette];
    twoTilesHi=memPPU[(((short int)nameTableEntry)<<4)+8+ palette];

    palettesLo=0xFF * (attrTableEntry&1);
    palettesHi=0xFF * ((attrTableEntry>>1)&1);

    //push first tile to make space for second
    twoTilesHi<<=8;
    twoTilesLo<<=8;
    palettesHi<<=8;
    palettesLo<<=8;

    //second tile
    nameTableEntry=memPPU[baseNT+1];
    attrTableEntry=memPPU[baseNT+0x3C0];
    twoTilesLo|=memPPU[(((int)nameTableEntry)<<4)+ palette];
    twoTilesHi|=memPPU[(((short int)nameTableEntry)<<4)+8+ palette];

    palettesLo|=0xFF * (attrTableEntry&1);
    palettesHi|=0xFF * ((attrTableEntry>>1)&1);

    for(i=0;i<341;i++)
    {
        ppuCycles+=1;
        waitForCPU();
    }

    //render visible scanlines
    for(i=0;i<240;i++)
        visibleScanline(i);

    //post render scanline
    for(i=0;i<341;i++)
    {
        ppuCycles+=1;
        waitForCPU();
    }

    memCPUWrite(0x2002,memCPURead(0x2002)|0x80);//Set vblank file


    //VBlank lines
    if(memCPURead(0x2000)&(1<<7))//check condition
        vblank=1;

    for(i=0;i<6820;i++)
    {
        ppuCycles+=1;
        waitForCPU();
    }
}


DWORD WINAPI mainPPU()
{
    while(ppuCycles<5360520*seconds)
    {
        processFrame();
        exampleScreen();
    }
    return 0;
}
