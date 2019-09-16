#include "stdio.h"
#include "windows.h"


unsigned short int pc;
char a=-1;
unsigned char ac,sp=0xFF,indRegX,indRegY,p=0;/*NV101IZC*/;
static unsigned short int adresa=0;
unsigned char memCPUi[65536];

void memCPUWrite(unsigned short int location, unsigned char data)
{
    if(location>=0x0800 && location<0x2000)
        location%=0x0800;
    else if(location>=0x2008 && location<0x4000)
        location=location%8+0x2000;

    static char first=1;
    static char strobe=0;
    memCPUi[location]=data;

    if(location == 0x4014)
    {
        int i;
        cycleCounter+=(cycleCounter%2?2:1);
        for(i=0;i<0x100;i++)
        {
            cycleCounter+=1;
            OAM[i]=memCPUi[data<<8|i];
            cycleCounter+=1;
        }
    }
    else if(location==0x4016)
    {
        if(data&1 && !strobe)
        {
            strobe=1;
        }
        else if(!(data&1) && strobe)
        {
            strobe=0;
            a=-1;
        }
    }
    else if(location == 0x2006)
    {
        if(first)
            adresa=data<<8;
        else
            adresa+=data;
        first=1-first;
    }
    else if(location == 0x2007)
    {
        memPPU[adresa%0x4000]=data;
        adresa+=(memCPUi[0x2000]&(1<<2)?32:1);
    }
    else if(location == 0x2003)
    {
        OAMADDR=data;
    }
    else if(location == 0x2004)
    {
        OAM[OAMADDR++]=data;
    }
    return;
}

unsigned char memCPURead(unsigned short int location)
{
    if(location>=0x0800 && location<0x2000)
        location%=0x0800;
    else if(location>=0x2008 && location<0x4000)
        location=location%8+0x2000;
    if(location == 0x2004)
    {
        return OAM[OAMADDR];
    }
    else if(location == 0x2002)
    {
        memCPUi[0x2006]=0;
        memCPUi[0x2005]=0;
    }
    else if(location == 0x2007)
    {
        unsigned short int stara=adresa;
        adresa+=(memCPUi[0x2000]&(1<<2)?32:1);
        return memPPU[stara%0x4000];
    }
    else if(location>=0x4000 && location<0x4020)
    {
        if(location==0x4016)
        {
            a++;
            if((a==0 && !(GetKeyState(0x46)&0x8000)) || (a==1 && !(GetKeyState(0x44)&0x8000)) || (a==2 && !(GetKeyState(0x41)&0x8000)) || (a==3 && !(GetKeyState(0x53)&0x8000)) ||
               (a==4 && !(GetKeyState(VK_UP)&0x8000)) || (a==5 && !(GetKeyState(VK_DOWN)&0x8000)) || (a==6 && !(GetKeyState(VK_LEFT)&0x8000)) || (a==7 && !(GetKeyState(VK_RIGHT)&0x8000)))
                return 0;
            return 1;
        }
    }
    return memCPUi[location];
}

void pushStack(unsigned char data)
{
    memCPUWrite(0x0100+(sp--),data);
}

void loadROM(char *filename)
{
    FILE *nesFile = fopen(filename,"rb");
    if(!nesFile)
        exit(100);
    unsigned char header[16];
    fread(header,sizeof(header),1,nesFile);
    fread(memCPUi+0x8000,sizeof(memCPUi[0]),(header[4]==2 ? 0x8000 :0x4000),nesFile);
    fread(memPPU,sizeof(memPPU[0]),0x2000,nesFile);
    for(pc=0;pc<0x4000 && header[4]==1;pc++)
        memCPUi[0xC000+pc]=memCPUi[0x8000+pc];
    cycleCounter+=7;
    p=0x24;
    pc=(short)(memCPURead(0xFFFD)<<8) | memCPURead(0xFFFC); //load pc from reset vector
    fclose(nesFile);
}

unsigned char pullStack()
{
    unsigned char pom=memCPURead(0x0100+(++sp));
    return pom;
}

unsigned char indirectIndex(unsigned char bite)
{
    return memCPURead((short)memCPURead((indRegX+bite+1)%256)<<8 | memCPURead((indRegX+bite)%256));
}

unsigned short indirectIndexA(unsigned char bite)
{
    return (short)memCPURead((indRegX+bite+1)%256)<<8 | memCPURead((indRegX+bite)%256);
}

unsigned char indexIndirect(unsigned char bite)
{
    return memCPURead(((short)memCPURead((bite+1)%256)<<8 | memCPURead(bite))+indRegY);
}

unsigned short indexIndirectA(unsigned char bite)
{
    return ((short)memCPURead((bite+1)%256)<<8 | memCPURead(bite))+indRegY;
}

unsigned char absolute(unsigned char bite1,unsigned char bite2)
{
    return memCPURead((short)bite2<<8 | bite1);
}

unsigned short absoluteA(unsigned char bite1,unsigned char bite2)
{
    return (short)bite2<<8 | bite1;
}

unsigned char indexZX(unsigned char bite)
{
    return memCPURead(bite+indRegX);
}

unsigned short indexZXA(unsigned char bite)
{
    return bite+indRegX;
}

unsigned char indexAbsX(unsigned char bite1,unsigned char bite2)
{
    return memCPURead(((short)bite2<<8 | bite1)+indRegX);
}

unsigned short indexAbsXA(unsigned char bite1,unsigned char bite2)
{
    return ((short)bite2<<8 | bite1)+indRegX;
}

unsigned char indexAbsY(unsigned char bite1,unsigned char bite2)
{
    return memCPURead((short)((short)bite2<<8 | bite1)+indRegY);
}

unsigned short indexAbsYA(unsigned char bite1,unsigned char bite2)
{
    return ((short)bite2<<8 | bite1)+indRegY;
}

void sta(unsigned short location)
{
    memCPUWrite(location,ac);
}

void stx(unsigned short location)
{
    memCPUWrite(location,indRegX);
}

void sty(unsigned short location)
{
    memCPUWrite(location,indRegY);
}

void lda(unsigned char data)
{
    ac=data;
    if(!ac)
        p|=(1<<1);//set zero flag
    else
        p&=~(1<<1);//clear zero flag
    if((signed char)ac<0)
        p|=(1<<7);//set negative flag
    else
        p&=~(1<<7);//clear negative flag
}

void ldx(unsigned char data)
{
    indRegX=data;
    if(!indRegX)
        p|=(1<<1);//set zero flag
    else
        p&=~(1<<1);//clear zero flag
    if((signed char)indRegX<0)
        p|=(1<<7);//set negative flag
    else
        p&=~(1<<7);//clear negative flag
}

void ldy(unsigned char data)
{
    indRegY=data;
    if(!indRegY)
        p|=(1<<1);//set zero flag
    else
        p&=~(1<<1);//clear zero flag
    if((signed char)indRegY<0)
        p|=(1<<7);//set negative flag
    else
        p&=~(1<<7);//clear negative flag
}

void compare(unsigned char data,unsigned char reg)
{
    unsigned char dif=reg-data;
    if(!dif)
        p|=(1<<1);//set zero flag
    else
        p&=~(1<<1);//clear zero flag
    if((signed char)dif<0)
        p|=(1<<7);//set negative flag
    else
        p&=~(1<<7);//clear negative flag
    if(reg>=data)
        p|=1;//set carry flag
    else
        p&=~1;//clear carry flag
}

void ora(unsigned char data)
{
    ac|=data;
    if(!ac)
        p|=(1<<1);//set zero flag
    else
        p&=~(1<<1);//clear zero flag
    if((signed char)ac<0)
        p|=(1<<7);//set negative flag
    else
        p&=~(1<<7);//clear negative flag
}

void and(unsigned char data)
{
    ac&=data;
    if(!ac)
        p|=(1<<1);//set zero flag
    else
        p&=~(1<<1);//clear zero flag
    if((signed char)ac<0)
        p|=(1<<7);//set negative flag
    else
        p&=~(1<<7);//clear negative flag

}

void eor(unsigned char data)
{
    ac^=data;
    if(!ac)
        p|=(1<<1);//set zero flag
    else
        p&=~(1<<1);//clear zero flag
    if((signed char)ac<0)
        p|=(1<<7);//set negative flag
    else
        p&=~(1<<7);//clear negative flag
}

void adc(unsigned char data)
{
    short int pom=ac+data+(p&1);
    if((ac^pom)&(data^pom)&0x80)
        p|=(1<<6);//set overflow flag
    else
        p&=~(1<<6);//clear overflow flag
    ac=pom;
    if(ac!=pom)
        p|=1;//set carry flag
    else
        p&=~1;//clear carry flag
    if(!ac)
        p|=(1<<1);//set zero flag
    else
        p&=~(1<<1);//clear zero flag
    if((signed char)ac<0)
        p|=(1<<7);//set negative flag
    else
        p&=~(1<<7);//clear negative flag
}

void sbc(unsigned char data)
{
    short int pom=ac-data-(~p&1);
    if((ac^pom)&(data^pom^0xff)&0x80)
        p|=(1<<6);//set overflow flag
    else
        p&=~(1<<6);//clear overflow flag
    ac=pom;
    if(pom>=0)
        p|=1;//set carry flag
    else
        p&=~1;//clear carry flag
    if(!ac)
        p|=(1<<1);//set zero flag
    else
        p&=~(1<<1);//clear zero flag
    if((signed char)ac<0)
        p|=(1<<7);//set negative flag
    else
        p&=~(1<<7);//clear negative flag
}

void asl(unsigned short location)
{
    if(memCPURead(location)>=0x80)
        p|=1;//set carry flag
    else
        p&=~1;//clear carry flag
    memCPUWrite(location,memCPURead(location)<<1);
    if(!memCPURead(location))
        p|=(1<<1);//set zero flag
    else
        p&=~(1<<1);//clear zero flag
    if((signed char)memCPURead(location)<0)
        p|=(1<<7);//set negative flag
    else
        p&=~(1<<7);//clear negative flag
}

void rol(unsigned short location)
{
    unsigned char pom=(p&1);
    if(memCPURead(location)>=0x80)
        p|=1;//set carry flag
    else
        p&=~1;//clear carry flag
    memCPUWrite(location,memCPURead(location)<<1);
    memCPUWrite(location,memCPURead(location)|pom);
    if(!memCPURead(location))
        p|=(1<<1);//set zero flag
    else
        p&=~(1<<1);//clear zero flag
    if((signed char)memCPURead(location)<0)
        p|=(1<<7);//set negative flag
    else
        p&=~(1<<7);//clear negative flag
}

void ror(unsigned short location)
{
    unsigned char pom=((p&1)<<7);
    if(memCPURead(location)&1)
        p|=1;//set carry flag
    else
        p&=~1;//clear carry flag
    memCPUWrite(location,memCPURead(location)>>1);
    memCPUWrite(location,memCPURead(location)|pom);
    if(!memCPURead(location))
        p|=(1<<1);//set zero flag
    else
        p&=~(1<<1);//clear zero flag
    if((signed char)memCPURead(location)<0)
        p|=(1<<7);//set negative flag
    else
        p&=~(1<<7);//clear negative flag
}

void lsr(unsigned short location)
{
    p&=~(1<<7);//clear negative flag
    if(memCPURead(location)&1)
        p|=1;//set carry flag
    else
        p&=~1;//clear carry flag
    memCPUWrite(location,memCPURead(location)>>1);
    if(!memCPURead(location))
        p|=(1<<1);//set zero flag
    else
        p&=~(1<<1);//clear zero flag
}

void dec(unsigned short location)
{
    memCPUWrite(location,memCPURead(location)-1);
    if(!memCPURead(location))
        p|=(1<<1);//set zero flag
    else
        p&=~(1<<1);//clear zero flag
    if((signed char)memCPURead(location)<0)
        p|=(1<<7);//set negative flag
    else
        p&=~(1<<7);//clear negative flag
}

void inc(unsigned short location)
{
    memCPUWrite(location,memCPURead(location)+1);
    if(!memCPURead(location))
        p|=(1<<1);//set zero flag
    else
        p&=~(1<<1);//clear zero flag
    if((signed char)memCPURead(location)<0)
        p|=(1<<7);//set negative flag
    else
        p&=~(1<<7);//clear negative flag
}

void aso(unsigned short location)
{
    asl(location);
    ora(memCPURead(location));
}

void rla(unsigned short location)
{
    rol(location);
    and(memCPURead(location));
}

void lse(unsigned short location)
{
    lsr(location);
    eor(memCPURead(location));
}

void rra(unsigned short location)
{
    ror(location);
    adc(memCPURead(location));
}

void axs(unsigned short location)
{
    memCPUWrite(location,ac&indRegX);
}

void lax(unsigned char data)
{
    lda(data);
    ldx(data);
}

void dcm(unsigned short location)
{
    dec(location);
    compare(memCPURead(location),ac);
}

void ins(unsigned short location)
{
    inc(location);
    sbc(memCPURead(location));
}

DWORD WINAPI mainCPU()
{
    unsigned char pom,pom2;

    while(cycleCounter<1786840*seconds)
    {
        if(vblank)
        {
            pushStack(pc>>8 & 0xFF); // push pc on stack
            pushStack(pc & 0xFF);
            pushStack(p|0x30); //push status flag on stack
            pc=(short)(memCPURead(0xFFFB)<<8) | memCPURead(0xFFFA);
            vblank=0;
        }

        while(ppuCycles<cycleCounter*3 || cycleCounter/44671>(clock()-t)/25);

        switch (memCPURead(pc))
        {
            case 0x00:

                p|=(1<<4); //set break flag
                pushStack(pc>>8 & 0xFF); // push pc on stack
                pushStack(pc & 0xFF);
                pushStack(p|0x30); //push status flag on stack

                pc=(short)memCPURead(0xFFFF)<<8 | memCPURead(0xFFFE); //load interrupt routine
                p|=(1<<2);//set interrupt flag
                cycleCounter+=7;
                break;
            case 0x01:
                pc++;

                ora(indirectIndex(memCPURead(pc)));
                cycleCounter+=6;
                break;
            case 0x05:
                pc++;

                ora(memCPURead(memCPURead(pc)));
                cycleCounter+=3;
                break;
            case 0x06:
                pc++;

                asl(memCPURead(pc));
                cycleCounter+=5;
                break;
            case 0x08:

                pushStack(p|0x30); //push status flag on stack
                cycleCounter+=3;
                break;
            case 0x09:
                pc++;

                ora(memCPURead(pc));
                cycleCounter+=2;
                break;

            case 0x0A:

                if(ac>=0x80)
                    p|=1;//set carry flag
                else
                    p&=~1;//clear carry flag
                ac<<=1;
                if(!ac)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                if((signed char)ac<0)
                    p|=(1<<7);//set negative flag
                else
                    p&=~(1<<7);//clear negative flag
                cycleCounter+=2;
                break;
            case 0x0D:
                pc++;

                ora(absolute(memCPURead(pc),memCPURead(pc+1)));
                pc++;
                cycleCounter+=4;
                break;
            case 0x0E:
                pc++;

                asl((short)memCPURead(pc+1)<<8 | memCPURead(pc));
                pc++;
                cycleCounter+=6;
                break;

            case 0x10:
                pc++;

                if((p&(1<<7))==0)//branch on N==0
                {
                    cycleCounter+=1+((pc&0xFF)==((memCPURead(pc)+pc)&0xFF));
                    pc+=(signed char)memCPURead(pc);
                }

                cycleCounter+=2;
                break;
            case 0x11:
                pc++;

                ora(indexIndirect(memCPURead(pc)));
                cycleCounter+=5;
                break;
            case 0x15:
                pc++;

                ora(indexZX(memCPURead(pc)));
                cycleCounter+=4;
                break;
            case 0x16:
                pc++;

                asl(memCPURead(pc)+indRegX);
                cycleCounter+=6;
                break;
            case 0x18:

                p&=~1;//reset clear flag
                cycleCounter+=2;
                break;
            case 0x19:
                pc++;

                ora(indexAbsY(memCPURead(pc),memCPURead(pc+1)));
                pc++;
                cycleCounter+=4;
                break;
            case 0x1D:
                pc++;

                ora(indexAbsX(memCPURead(pc),memCPURead(pc+1)));
                pc++;
                cycleCounter+=4;
                break;
            case 0x1E:
                pc++;

                asl(((short)memCPURead(pc+1)<<8 | memCPURead(pc))+indRegX);
                pc++;
                cycleCounter+=7;
                break;

            case 0x20:
                pc++;

                pushStack((pc+1)>>8 & 0xFF); // push pc on stack
                pushStack((pc+1) & 0xFF);
                pc=(short)memCPURead(pc+1)<<8 | memCPURead(pc);
                cycleCounter+=6;
                pc--;
                break;
            case 0x21:
                pc++;

                and(indirectIndex(memCPURead(pc)));
                cycleCounter+=6;
                break;
            case 0x24:
                pc++;

                if(memCPURead(memCPURead(pc))&(1<<7))
                    p|=(1<<7);
                else
                    p&=~(1<<7);
                if(memCPURead(memCPURead(pc))&(1<<6))
                    p|=(1<<6);
                else
                    p&=~(1<<6);
                if((memCPURead(memCPURead(pc))&ac)==0)
                    p|=(1<<1);
                else
                    p&=~(1<<1);
                cycleCounter+=3;
                break;
            case 0x25:
                pc++;

                and(memCPURead(memCPURead(pc)));
                cycleCounter+=3;
                break;
            case 0x26:
                pc++;

                rol(memCPURead(pc));
                cycleCounter+=5;
                break;
            case 0x28:

                p=pullStack()&0xEF;
                p|=0x20;
                cycleCounter+=4;
                break;
            case 0x29:
                pc++;

                and(memCPURead(pc));
                cycleCounter+=2;
                break;
            case 0x2A:

                pom=(p&1);
                if(ac>=0x80)
                    p|=1;//set carry flag
                else
                    p&=~1;//clear carry flag
                ac<<=1;
                ac|=pom;
                if(!ac)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                if((signed char)ac<0)
                    p|=(1<<7);//set negative flag
                else
                    p&=~(1<<7);//clear negative flag
                cycleCounter+=2;
                break;
            case 0x2C:
                pc++;

                if(absolute(memCPURead(pc),memCPURead(pc+1))&(1<<7))
                    p|=(1<<7);
                else
                    p&=~(1<<7);
                if(absolute(memCPURead(pc),memCPURead(pc+1))&(1<<6))
                    p|=(1<<6);
                else
                    p&=~(1<<6);
                if((absolute(memCPURead(pc),memCPURead(pc+1))&ac)==0)
                    p|=(1<<1);
                else
                    p&=~(1<<1);
                pc++;
                cycleCounter+=4;
                break;
            case 0x2D:
                pc++;

                and(absolute(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0x2E:
                pc++;

                rol((short)memCPURead(pc+1)<<8 | memCPURead(pc));
                cycleCounter+=6;
                pc++;
                break;

            case 0x30:
                pc++;

                if((p&(1<<7)))//branch on N==1
                {
                    cycleCounter+=1+((pc&0xFF)==((memCPURead(pc)+pc)&0xFF));
                    pc+=(signed char)memCPURead(pc);
                }

                cycleCounter+=2;
                break;
            case 0x31:
                pc++;

                and(indexIndirect(memCPURead(pc)));
                cycleCounter+=5;
                break;
            case 0x35:
                pc++;

                and(memCPURead(memCPURead(pc)+indRegX));
                cycleCounter+=4;
                break;
            case 0x36:
                pc++;

                rol(memCPURead(pc)+indRegX);
                cycleCounter+=5;
                break;
            case 0x38:

                p|=1;
                cycleCounter+=2;
                break;
            case 0x39:
                pc++;

                and(indexAbsY(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0x3D:
                pc++;

                and(indexAbsX(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0x3E:
                pc++;

                rol(((short)memCPURead(pc+1)<<8 |memCPURead(pc))+indRegX);
                pc++;
                cycleCounter+=7;
                break;

            case 0x40:

                p=pullStack()&0xEF;
                p|=0x20;
                pc=pullStack();
                pc |= (short)pullStack()<<8;
                cycleCounter+=6;
                pc--;
                break;
            case 0x41:
                pc++;

                eor(indirectIndex(memCPURead(pc)));
                cycleCounter+=6;
                break;
            case 0x45:
                pc++;

                eor(memCPURead(memCPURead(pc)));
                cycleCounter+=3;
                break;
            case 0x46:
                pc++;

                lsr(memCPURead(pc));
                cycleCounter+=5;
                break;
            case 0x48:

                pushStack(ac);
                cycleCounter+=3;
                break;
            case 0x49:
                pc++;

                eor(memCPURead(pc));
                cycleCounter+=2;
                break;
            case 0x4A:

                p&=~(1<<7);//mozda gore???
                if(ac&1)
                    p|=1;//set carry flag
                else
                    p&=~1;//clear carry flag
                ac>>=1;
                if(!ac)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                cycleCounter+=2;
                break;
            case 0x4C:
                pc++;

                pc=(short)memCPURead(pc+1)<<8 | memCPURead(pc);
                cycleCounter+=3;
                pc--;
                break;
            case 0x4D:
                pc++;

                eor(absolute(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0x4E:
                pc++;

                lsr((short)memCPURead(pc+1)<<8 | memCPURead(pc));
                cycleCounter+=6;
                pc++;
                break;

            case 0x50:
                pc++;

                if((p&(1<<6))==0)//branch on N==0
                {
                    cycleCounter+=1+((pc&0xFF)==((memCPURead(pc)+pc)&0xFF));
                    pc+=(signed char)memCPURead(pc);
                }

                cycleCounter+=2;
                break;
            case 0x51:
                pc++;

                eor(indexIndirect(memCPURead(pc)));
                cycleCounter+=5;
                break;
            case 0x55:
                pc++;

                eor(memCPURead(memCPURead(pc)+indRegX));
                cycleCounter+=4;
                break;
            case 0x56:
                pc++;

                lsr(memCPURead(pc)+indRegX);
                cycleCounter+=6;
                break;
            case 0x58:

                p&=~(1<<2);//reset clear flag
                cycleCounter+=2;
                break;
            case 0x59:
                pc++;

                eor(indexAbsY(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0x5D:
                pc++;

                eor(indexAbsX(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0x5E:
                pc++;

                lsr(((short)memCPURead(pc+1)<<8 | memCPURead(pc))+indRegX);
                cycleCounter+=7;
                pc++;
                break;

            case 0x60:

                pc=pullStack() | (short)pullStack()<<8;
                cycleCounter+=6;
                break;
            case 0x61:
                pc++;

                adc(indirectIndex(memCPURead(pc)));
                cycleCounter+=6;
                break;
            case 0x65:
                pc++;

                adc(memCPURead(memCPURead(pc)));
                cycleCounter+=3;
                break;
            case 0x66:
                pc++;

                ror(memCPURead(pc));
                cycleCounter+=5;
                break;
            case 0x68:

                ac=pullStack();
                if(!ac)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                if((signed char)ac<0)
                    p|=(1<<7);//set negative flag
                else
                    p&=~(1<<7);//clear negative flag
                cycleCounter+=4;
                break;
            case 0x69:
                pc++;

                adc(memCPURead(pc));
                cycleCounter+=2;
                break;
            case 0x6A:

                pom2=((p&1)<<7);
                if(ac&1)
                    p|=1;//set carry flag
                else
                    p&=~1;//clear carry flag
                ac>>=1;
                ac|=pom2;
                if(!ac)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                if((signed char)ac<0)
                    p|=(1<<7);//set negative flag
                else
                    p&=~(1<<7);//clear negative flag
                cycleCounter+=2;
                break;
            case 0x6C:
                pc++;

                pc=memCPURead(memCPURead(pc) | (short)(memCPURead(pc+1))<<8) | memCPURead(((unsigned char)(memCPURead(pc)+1) ) | (short)(memCPURead(pc+1)<<8))<<8;
                cycleCounter+=5;
                pc--;
                break;
            case 0x6D:
                pc++;

                adc(absolute(memCPURead(pc),memCPURead(pc+1)));
                pc++;
                cycleCounter+=4;
                break;
            case 0x6E:
                pc++;

                ror(memCPURead(pc) | (short)(memCPURead(pc+1)<<8));
                pc++;
                cycleCounter+=6;
                break;

            case 0x70:
                pc++;

                if((p&(1<<6)))//branch on V==1
                {
                    cycleCounter+=1+((pc&0xFF)==((memCPURead(pc)+pc)&0xFF));
                    pc+=(signed char)memCPURead(pc);
                }

                cycleCounter+=2;
                break;
            case 0x71:
                pc++;

                adc(indexIndirect(memCPURead(pc)));
                cycleCounter+=5;
                break;
            case 0x75:
                pc++;

                adc(memCPURead(memCPURead(pc)+indRegX));
                cycleCounter+=4;
                break;
            case 0x76:
                pc++;

                ror(memCPURead(pc)+indRegX);
                cycleCounter+=6;
                break;
            case 0x78:

                p|=(1<<2);
                cycleCounter+=2;
                break;
            case 0x79:
                pc++;

                adc(indexAbsY(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0x7D:
                pc++;

                adc(indexAbsX(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0x7E:
                pc++;

                ror((memCPURead(pc) | (short)(memCPURead(pc+1)<<8))+indRegX);
                pc++;
                cycleCounter+=7;
                break;

            case 0x81:
                pc++;

                sta(indirectIndexA(memCPURead(pc)));
                cycleCounter+=6;
                break;
            case 0x84:
                pc++;

                sty(memCPURead(pc));
                cycleCounter+=3;
                break;
            case 0x85:
                pc++;

                sta(memCPURead(pc));
                cycleCounter+=3;
                break;
            case 0x86:
                pc++;


                stx(memCPURead(pc));
                cycleCounter+=3;
                break;
            case 0x88:

                indRegY--;
                if(!indRegY)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                if((signed char)indRegY<0)
                    p|=(1<<7);//set negative flag
                else
                    p&=~(1<<7);//clear negative flag
                cycleCounter+=2;
                break;
            case 0x8A:

                ac=indRegX;
                if(!ac)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                if((signed char)ac<0)
                    p|=(1<<7);//set negative flag
                else
                    p&=~(1<<7);//clear negative flag
                cycleCounter+=2;
                break;
            case 0x8C:
                pc++;

                sty(absoluteA(memCPURead(pc),memCPURead(pc+1)));
                pc++;
                cycleCounter+=4;
                break;
            case 0x8D:
                pc++;

                sta(absoluteA(memCPURead(pc),memCPURead(pc+1)));
                pc++;
                cycleCounter+=4;
                break;
            case 0x8E:
                pc++;

                stx(absoluteA(memCPURead(pc),memCPURead(pc+1)));
                pc++;
                cycleCounter+=4;
                break;

            case 0x90:
                pc++;

                if((p&1)==0)//branch on C==0
                {
                    cycleCounter+=1+((pc&0xFF)==((memCPURead(pc)+pc)&0xFF));
                    pc+=(signed char)memCPURead(pc);
                }

                cycleCounter+=2;
                break;
            case 0x91:
                pc++;

                sta(indexIndirectA(memCPURead(pc)));
                cycleCounter+=6;
                break;
            case 0x94:
                pc++;

                sty((memCPURead(pc)+indRegX)%256);
                cycleCounter+=4;
                break;
            case 0x95:
                pc++;

                sta((memCPURead(pc)+indRegX)%256);
                cycleCounter+=4;
                break;
            case 0x96:
                pc++;

                stx((memCPURead(pc)+indRegY)%256);
                cycleCounter+=4;
                break;
            case 0x98:

                ac=indRegY;
                if(!ac)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                if((signed char)ac<0)
                    p|=(1<<7);//set negative flag
                else
                    p&=~(1<<7);//clear negative flag
                cycleCounter+=2;
                break;
            case 0x99:
                pc++;

                sta(indexAbsYA(memCPURead(pc),memCPURead(pc+1)));
                pc++;
                cycleCounter+=5;
                break;
            case 0x9A:

                sp=indRegX;
                cycleCounter+=2;
                break;
            case 0x9D:
                pc++;

                sta(indexAbsXA(memCPURead(pc),memCPURead(pc+1)));
                pc++;
                break;

            case 0xA0:
                pc++;

                ldy(memCPURead(pc));
                cycleCounter+=2;
                break;
            case 0xA1:
                pc++;

                lda(indirectIndex(memCPURead(pc)));
                cycleCounter+=6;
                break;
            case 0xA2:
                pc++;

                ldx(memCPURead(pc));
                cycleCounter+=2;
                break;
            case 0xA4:
                pc++;

                ldy(memCPURead(memCPURead(pc)));
                cycleCounter+=3;
                break;
            case 0xA5:
                pc++;

                lda(memCPURead(memCPURead(pc)));
                cycleCounter+=3;
                break;
            case 0xA6:
                pc++;

                ldx(memCPURead(memCPURead(pc)));
                cycleCounter+=3;
                break;
            case 0xA8:

                indRegY=ac;
                if(!ac)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                if((signed char)ac<0)
                    p|=(1<<7);//set negative flag
                else
                    p&=~(1<<7);//clear negative flag
                cycleCounter+=2;
                break;
            case 0xA9:
                pc++;

                lda(memCPURead(pc));
                cycleCounter+=2;
                break;
            case 0xAA:

                indRegX=ac;
                if(!ac)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                if((signed char)ac<0)
                    p|=(1<<7);//set negative flag
                else
                    p&=~(1<<7);//clear negative flag
                cycleCounter+=2;
                break;
            case 0xAC:
                pc++;

                ldy(absolute(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0xAD:
                pc++;

                lda(absolute(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0xAE:
                pc++;

                ldx(absolute(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;

            case 0xB0:
                pc++;

                if(p&1)//branch on C==1
                {
                    cycleCounter+=1+((pc&0xFF)==((memCPURead(pc)+pc)&0xFF));
                    pc+=(signed char)memCPURead(pc);
                }

                cycleCounter+=2;
                break;
            case 0xB1:
                pc++;

                lda(indexIndirect(memCPURead(pc)));
                cycleCounter+=5;
                break;
            case 0xB4:
                pc++;

                ldy(memCPURead((memCPURead(pc)+indRegX)%256));
                cycleCounter+=4;
                break;
            case 0xB5:
                pc++;

                lda(memCPURead((memCPURead(pc)+indRegX)%256));
                cycleCounter+=4;
                break;
            case 0xB6:
                pc++;

                ldx(memCPURead((memCPURead(pc)+indRegY)%256));
                cycleCounter+=4;
                break;
            case 0xB8:

                p&=~(1<<6);//reset overflow flag
                cycleCounter+=2;
                break;
            case 0xB9:
                pc++;

                lda(indexAbsY(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0xBA:

                indRegX=sp;
                if(!sp)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                if((signed char)sp<0)
                    p|=(1<<7);//set negative flag
                else
                    p&=~(1<<7);//clear negative flag
                cycleCounter+=2;
                break;
            case 0xBC:
                pc++;

                ldy(indexAbsX(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0xBD:
                pc++;

                lda(indexAbsX(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0xBE:
                pc++;

                ldx(indexAbsY(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;

            case 0xC0:
                pc++;

                compare(memCPURead(pc),indRegY);
                cycleCounter+=2;
                break;
            case 0xC1:
                pc++;

                compare(indirectIndex(memCPURead(pc)),ac);
                cycleCounter+=6;
                break;
            case 0xC4:
                pc++;

                compare(memCPURead(memCPURead(pc)),indRegY);
                cycleCounter+=3;
                break;
            case 0xC5:
                pc++;

                compare(memCPURead(memCPURead(pc)),ac);
                cycleCounter+=3;
                break;
            case 0xC6:
                pc++;

                dec(memCPURead(pc));
                cycleCounter+=5;
                break;
            case 0xC8:

                indRegY++;
                if(!indRegY)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                if((signed char)indRegY<0)
                    p|=(1<<7);//set negative flag
                else
                    p&=~(1<<7);//clear negative flag
                cycleCounter+=2;
                break;
            case 0xC9:
                pc++;

                compare(memCPURead(pc),ac);
                cycleCounter+=2;
                break;
            case 0xCA:

                indRegX--;
                if(!indRegX)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                if((signed char)indRegX<0)
                    p|=(1<<7);//set negative flag
                else
                    p&=~(1<<7);//clear negative flag
                cycleCounter+=2;
                break;
            case 0xCC:
                pc++;

                compare(absolute(memCPURead(pc),memCPURead(pc+1)),indRegY);
                cycleCounter+=4;
                pc++;
                break;
            case 0xCD:
                pc++;


                compare(absolute(memCPURead(pc),memCPURead(pc+1)),ac);
                cycleCounter+=4;
                pc++;
                break;
            case 0xCE:
                pc++;

                dec(absoluteA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;

            case 0xD0:
                pc++;

                if((p&(1<<1))==0)
                {
                    cycleCounter+=1+((pc&0xFF)==((memCPURead(pc)+pc)&0xFF));
                    pc+=(signed char)memCPURead(pc);
                }

                cycleCounter+=2;
                break;
            case 0xD1:
                pc++;

                compare(indexIndirect(memCPURead(pc)),ac);
                cycleCounter+=5;
                break;
            case 0xD5:
                pc++;

                compare(memCPURead(memCPURead(pc)+indRegX),ac);
                cycleCounter+=4;
                break;
            case 0xD6:
                pc++;

                dec(memCPURead(pc)+indRegX);
                cycleCounter+=6;
                break;
            case 0xD8:

                p&=~(1<<3);//clear decimal flag
                cycleCounter+=2;
                break;
            case 0xD9:
                pc++;

                compare(indexAbsY(memCPURead(pc),memCPURead(pc+1)),ac);
                cycleCounter+=4;
                pc++;
                break;
            case 0xDD:
                pc++;

                compare(indexAbsX(memCPURead(pc),memCPURead(pc+1)),ac);
                cycleCounter+=4;
                pc++;
                break;
                pc++;
            case 0xDE:
                pc++;

                dec(indexAbsXA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=7;
                pc++;
                break;

            case 0xE0:
                pc++;

                compare(memCPURead(pc),indRegX);
                cycleCounter+=2;
                break;
            case 0xE1:
                pc++;

                sbc(indirectIndex(memCPURead(pc)));
                cycleCounter+=6;
                break;
            case 0xE4:
                pc++;

                compare(memCPURead(memCPURead(pc)),indRegX);
                cycleCounter+=3;
                break;
            case 0xE5:
                pc++;

                sbc(memCPURead(memCPURead(pc)));
                cycleCounter+=3;
                break;
            case 0xE6:
                pc++;

                inc(memCPURead(pc));
                cycleCounter+=5;
                break;
            case 0xE8:

                indRegX++;
                if(!indRegX)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                if((signed char)indRegX<0)
                    p|=(1<<7);//set negative flag
                else
                    p&=~(1<<7);//clear negative flag
                cycleCounter+=2;
                break;
            case 0xE9:
                pc++;

                sbc(memCPURead(pc));
                cycleCounter+=2;
                break;
            case 0xEA:

                cycleCounter+=2;
                break;
            case 0xEC:
                pc++;

                compare(absolute(memCPURead(pc),memCPURead(pc+1)),indRegX);
                cycleCounter+=4;
                pc++;
                break;
            case 0xED:
                pc++;

                sbc(absolute(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0xEE:
                pc++;

                inc(absoluteA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=6;
                pc++;
                break;

            case 0xF0:
                pc++;

                if((p&(1<<1)))//branch on Z==1
                {
                    cycleCounter+=1+((pc&0xFF)==((memCPURead(pc)+pc)&0xFF));
                    pc+=(signed char)memCPURead(pc);
                }

                cycleCounter+=2;
                break;
            case 0xF1:
                pc++;

                sbc(indexIndirect(memCPURead(pc)));
                cycleCounter+=5;
                break;
            case 0xF5:
                pc++;

                sbc(memCPURead(memCPURead(pc)+indRegX));
                cycleCounter+=4;
                break;
            case 0xF6:
                pc++;

                inc(memCPURead(pc)+indRegX);
                cycleCounter+=6;
                break;
            case 0xF8:

                p|=(1<<3);
                cycleCounter+=2;
                break;
            case 0xF9:
                pc++;

                sbc(indexAbsY(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0xFD:
                pc++;

                sbc(indexAbsX(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0xFE:
                pc++;

                inc(indexAbsXA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=7;
                pc++;
                break;

            //UNOFFICIAL

            //ASO
            case 0x03:
                pc++;

                aso(indirectIndexA(memCPURead(pc)));
                cycleCounter+=8;
                break;
            case 0x07:
                pc++;

                aso(memCPURead(pc));
                cycleCounter+=5;
                break;
            case 0x0F:
                pc++;

                aso(absoluteA(memCPURead(pc),memCPURead(pc+1)));
                pc++;
                cycleCounter+=6;
                break;
            case 0x13:
                pc++;

                aso(indexIndirectA(memCPURead(pc)));
                cycleCounter+=8;
                break;
            case 0x17:
                pc++;

                aso((memCPURead(pc)+indRegX)%256);
                cycleCounter+=6;
                break;
            case 0x1B:
                pc++;

                aso(indexAbsYA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=7;
                pc++;
                break;
            case 0x1F:
                pc++;

                aso(indexAbsXA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=7;
                pc++;
                break;

            //RLA
            case 0x23:
                pc++;

                rla(indirectIndexA(memCPURead(pc)));
                cycleCounter+=8;
                break;
            case 0x27:
                pc++;

                rla(memCPURead(pc));
                cycleCounter+=5;
                break;
            case 0x2F:
                pc++;

                rla(absoluteA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=6;
                pc++;
                break;
            case 0x33:
                pc++;

                rla(indexIndirectA(memCPURead(pc)));
                cycleCounter+=6;
                break;
            case 0x37:
                pc++;

                rla((memCPURead(pc)+indRegX)%256);
                cycleCounter+=6;
                break;
            case 0x3B:
                pc++;

                rla(indexAbsYA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=7;
                pc++;
                break;
            case 0x3F:
                pc++;

                rla(indexAbsXA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=7;
                pc++;
                break;

            //LSE
            case 0x43:
                pc++;

                lse(indirectIndexA(memCPURead(pc)));
                cycleCounter+=8;
                break;
            case 0x47:
                pc++;

                lse(memCPURead(pc));
                cycleCounter+=5;
                break;
            case 0x4F:
                pc++;

                lse(absoluteA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=6;
                pc++;
                break;
            case 0x53:
                pc++;

                lse(indexIndirectA(memCPURead(pc)));
                cycleCounter+=6;
                break;
            case 0x57:
                pc++;

                lse((memCPURead(pc)+indRegX)%256);
                cycleCounter+=6;
                break;
            case 0x5B:
                pc++;

                lse(indexAbsYA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=7;
                pc++;
                break;
            case 0x5F:
                pc++;

                lse(indexAbsXA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=7;
                pc++;
                break;

            //RRA
            case 0x63:
                pc++;

                rra(indirectIndexA(memCPURead(pc)));
                cycleCounter+=8;
                break;
            case 0x67:
                pc++;

                rra(memCPURead(pc));
                cycleCounter+=5;
                break;
            case 0x6F:
                pc++;

                rra(absoluteA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=6;
                pc++;
                break;
            case 0x73:
                pc++;

                rra(indexIndirectA(memCPURead(pc)));
                cycleCounter+=8;
                break;
            case 0x77:
                pc++;

                rra((memCPURead(pc)+indRegX)%256);
                cycleCounter+=6;
                break;
            case 0x7B:
                pc++;

                rra(indexAbsYA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=7;
                pc++;
                break;
            case 0x7F:
                pc++;

                rra(indexAbsXA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=7;
                pc++;
                break;

            //AXS
            case 0x83:
                pc++;

                axs(indirectIndexA(memCPURead(pc)));
                cycleCounter+=6;
                break;
            case 0x87:
                pc++;

                axs(memCPURead(pc));
                cycleCounter+=3;
                break;
            case 0x8F:
                pc++;

                axs(absoluteA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0x97:
                pc++;

                axs((memCPURead(pc)+indRegY)%256);
                cycleCounter+=4;
                break;

            //LAX
            case 0xA3:
                pc++;

                lax(indirectIndex(memCPURead(pc)));
                cycleCounter+=6;
                break;
            case 0xA7:
                pc++;

                lax(memCPURead(memCPURead(pc)));
                cycleCounter+=3;
                break;
            case 0xAF:
                pc++;

                lax(absolute(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;
            case 0xB3:
                pc++;

                lax(indexIndirect(memCPURead(pc)));
                cycleCounter+=5;
                break;
            case 0xB7:
                pc++;

                lax(memCPURead((memCPURead(pc)+indRegY)%256));
                cycleCounter+=4;
                break;
            case 0xBF:
                pc++;

                lax(indexAbsY(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=4;
                pc++;
                break;

            //DCM
            case 0xC3:
                pc++;

                dcm(indirectIndexA(memCPURead(pc)));
                cycleCounter+=8;
                break;
            case 0xC7:
                pc++;

                dcm(memCPURead(pc));
                cycleCounter+=5;
                break;
            case 0xCF:
                pc++;

                dcm(absoluteA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=6;
                pc++;
                break;
            case 0xD3:
                pc++;

                dcm(indexIndirectA(memCPURead(pc)));
                cycleCounter+=8;
                break;
            case 0xD7:
                pc++;

                dcm((memCPURead(pc)+indRegX)%256);
                cycleCounter+=6;
                break;
            case 0xDB:
                pc++;

                dcm(indexAbsYA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=7;
                pc++;
                break;
            case 0xDF:
                pc++;

                dcm(indexAbsXA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=7;
                pc++;
                break;

            //INS
            case 0xE3:
                pc++;

                ins(indirectIndexA(memCPURead(pc)));
                cycleCounter+=8;
                break;
            case 0xE7:
                pc++;

                ins(memCPURead(pc));
                cycleCounter+=5;
                break;
            case 0xEF:
                pc++;

                ins(absoluteA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=6;
                pc++;
                break;
            case 0xF3:
                pc++;

                ins(indexIndirectA(memCPURead(pc)));
                cycleCounter+=8;
                break;
            case 0xF7:
                pc++;

                ins((memCPURead(pc)+indRegX)%256);
                cycleCounter+=6;
                break;
            case 0xFB:
                pc++;

                ins(indexAbsYA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=7;
                pc++;
                break;
            case 0xFF:
                pc++;

                ins(indexAbsXA(memCPURead(pc),memCPURead(pc+1)));
                cycleCounter+=7;
                pc++;
                break;

            //one adressing instructions
            case 0x4B:
                pc++;

                and(memCPURead(pc));
                p&=~(1<<7);//mozda gore???
                if(ac&1)
                    p|=1;//set carry flag
                else
                    p&=~1;//clear carry flag
                ac>>=1;
                if(!ac)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                cycleCounter+=2;
                break;
            case 0x6B:
                pc++;

                and(memCPURead(pc));
                unsigned char pom3=((p&1)<<7);
                if(ac&1)
                    p|=1;//set carry flag
                else
                    p&=~1;//clear carry flag
                ac>>=1;
                ac|=pom3;
                if(!ac)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                if((signed char)ac<0)
                    p|=(1<<7);//set negative flag
                else
                    p&=~(1<<7);//clear negative flag
                cycleCounter+=2;
                break;
            case 0x8B:
                pc++;

                ac=indRegX;
                and(memCPURead(pc));
                cycleCounter+=2;
                break;
            case 0xAB:
                pc++;

                ora(0xee);
                and(memCPURead(pc));
                indRegX=ac;
                if(!ac)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                if((signed char)ac<0)
                    p|=(1<<7);//set negative flag
                else
                    p&=~(1<<7);//clear negative flag
                cycleCounter+=2;
                break;
            case 0xCB:
                pc++;

                unsigned char dif=(ac&indRegX)-memCPURead(pc);
                if(!dif)
                    p|=(1<<1);//set zero flag
                else
                    p&=~(1<<1);//clear zero flag
                if((signed char)dif<0)
                    p|=(1<<7);//set negative flag
                else
                    p&=~(1<<7);//clear negative flag
                if(ac&indRegX>=memCPURead(pc))
                    p|=1;//set carry flag
                else
                    p&=~1;//clear carry flag
                indRegX=dif;
                cycleCounter+=2;
                break;

            //HLT
            case 0x02:
            case 0x12:
            case 0x22:
            case 0x32:
            case 0x42:
            case 0x52:
            case 0x62:
            case 0x72:
            case 0x92:
            case 0xB2:
            case 0xD2:
            case 0xF2:
                break;

            //More one adress instructions(absolute, no register interaction)
            case 0x9B:
                pc++;

                sp=ac&indRegX;
                memCPUWrite(indexAbsYA(memCPURead(pc),memCPURead(pc+1)),sp&((memCPURead(pc+1)+1)%256));
                pc++;
                cycleCounter+=5;
                break;
            case 0x9C:
                pc++;

                memCPUWrite(indexAbsXA(memCPURead(pc),memCPURead(pc+1)),indRegY&((memCPURead(pc+1)+1)%256));
                pc++;
                cycleCounter+=5;
                break;
            case 0x9E:
                pc++;

                memCPUWrite(indexAbsYA(memCPURead(pc),memCPURead(pc+1)),indRegX&((memCPURead(pc+1)+1)%256));
                pc++;
                cycleCounter+=5;
                break;
            case 0x93:
                pc++;

                memCPUWrite(indexIndirectA(memCPURead(pc)),ac&indRegX&((memCPURead(pc)+1)%256));
                cycleCounter+=6;
                break;
            case 0x9F:
                pc++;

                memCPUWrite(indexAbsYA(memCPURead(pc),memCPURead(pc+1)),ac&indRegX&((memCPURead(pc+1)+1)%256));
                pc++;
                cycleCounter+=5;
                break;

            //some miscellanious instructions
            case 0x0B:
            case 0x2B:
                pc++;

                and(memCPURead(pc));
                if(p>>7)
                    p|=1;//set carry flag
                else
                    p&=~1;//clear carry flag
                cycleCounter+=2;
                break;
            case 0xBB:
                pc++;

                sp&=indexAbsY(memCPURead(pc),memCPURead(pc+1));
                lda(sp);
                ldx(sp);
                ldy(sp);
                pc++;
                cycleCounter+=4;
                break;
            case 0xEB:
                pc++;

                sbc(memCPURead(pc));
                cycleCounter+=2;
                break;

            //NOP
            case 0x1C:
            case 0x3C:
            case 0x5C:
            case 0x7C:
            case 0xdC:
            case 0xfC:
                cycleCounter+=1;

            case 0x0C:
                pc++;
                cycleCounter+=4;
                pc++;
                break;

            case 0x1A:
            case 0x3A:
            case 0x5A:
            case 0x7A:
            case 0xdA:
            case 0xfA:
                cycleCounter+=2;
                break;

            case 0x80:
            case 0x82:
            case 0x89:
            case 0xC2:
            case 0xE2:
                cycleCounter-=1;
            case 0x04:
            case 0x44:
            case 0x64:
                cycleCounter-=1;
            case 0x14:
            case 0x34:
            case 0x54:
            case 0x74:
            case 0xd4:
            case 0xf4:
                pc++;

                cycleCounter+=4;
                break;
            default:
                break;
        }
        pc++;
    }
    return 0;
}
