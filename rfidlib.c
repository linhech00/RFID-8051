#include "rfidlib.h"

void delay_u(int i)
{
    while(i--);
}

unsigned char MFRC522_Rd(unsigned char Address)
{
    unsigned int i,ucAddr;
    unsigned int ucResult = 0;
    MFRC522_SCK = 0;
    MFRC522_CS = 0;
    ucAddr = ((Address<<1)&0x7E)|0x80;

    for(i=8;i>0;i--)
    {
        MFRC522_SI = (ucAddr&0x80)==0x80;
        MFRC522_SCK = 1;
        ucAddr<<=1;
        MFRC522_SCK = 0;
    }

    for(i=8;i>0;i--)
    {
        MFRC522_SCK = 1;
        ucResult <<=1;
        ucResult |= MFRC522_SO;
        MFRC522_SCK = 0;
    }

    MFRC522_CS = 1;
    MFRC522_SCK = 1;
    return ucResult;
}

void MFRC522_Wr(unsigned char Address, unsigned char value)
{
    unsigned char i, ucAddr;
    MFRC522_SCK = 0;
    MFRC522_CS = 0;
    ucAddr = ((Address<<1)&0x7E);

    for(i=8;i>0;i--)
    {
        MFRC522_SI = (ucAddr&0x80)==0x80;
        MFRC522_SCK = 1;
        ucAddr<<=1;
        MFRC522_SCK = 0;
    }

    for(i = 8;i>0;i++)
    {
        MFRC522_SI = (value&0x80)==0x80;
        MFRC522_SCK = 1;
        value <<=1;
        MFRC522_SCK = 0;
    }

    MFRC522_CS = 1;
    MFRC522_SCK = 1;
}

void MFRC522_Clear_Bit(char addr, char mask)
{
    unsigned char tmp = 0x00;
    tmp = MFRC522_Rd(addr);
    MFRC522_Wr(addr,tmp&(~mask));
}

void MFRC522_Set_Bit(char addr, char mask)
{
    unsigned char tmp = 0x00;
    tmp = MFRC522_Rd(addr);
    MFRC522_Wr(addr,tmp|mask);
}

void MFRC522_Reset()
{
    MFRC522_RST = 1;
    delay_u(1);
    MFRC522_RST = 0;
    delay_u(1);
    MFRC522_RST = 1;
    delay_u(1);
    MFRC522_Wr(COMMANDREG,PCD_RESETPHASE);
    delay_u(1);
}

void MFRC522_AntennaOn()
{
    unsigned char stt;
    stt = MFRC522_Rd(TXCONTROLREG);
    if(!(stt & 0x03))
    {
        MFRC522_Set_Bit(TXCONTROLREG,0x03);
    }
}

void MFRC522_AntennaOff()
{
    MFRC522_Clear_Bit(TXCONTROLREG,0x03);
}

void MFRC522_Init()
{
    MFRC522_CS = 1;
    MFRC522_RST = 1;

    MFRC522_Reset();
    MFRC522_Wr(TMODEREG,0x8d);
    MFRC522_Wr(TPRESCALERREG,0x3e);
    MFRC522_Wr(TRELOADREGL,30);
    MFRC522_Wr(TRELOADREGH,0);
    MFRC522_Wr(TXAUTOREG,0x40);
    MFRC522_Wr(MODEREG,0x3d);

    MFRC522_AntennaOff();
    MFRC522_AntennaOn();
}

char MFRC522_ToCard(char command, char* sendData, char sendlen, char* backData, char* backlen)
{
    char _status = MI_ERR;
    char irqEn = 0x00;
    char waitIRq = 0x00;
    char lastBits;
    char n;
    unsigned int i;

    switch (command)
    {
    case PCD_AUTHENT:
    {
        irqEn = 0x12;
        waitIRq = 0x10;
        break;
    }
    case PCD_TRANSCEIVE:
    {
        irqEn = 0x77;
        waitIRq = 0x30;
        break;
    }
    default:
        break;
    }
    MFRC522_Wr(COMMIENREG, irqEn|0x80);
    MFRC522_Clear_Bit(COMMIRQREG,0x80);
    MFRC522_Set_Bit(FIFOLEVELREG,0x80);
    MFRC522_Wr(COMMANDREG,PCD_IDLE);

    for(i= 0;i<sendlen;i++)
    {
        MFRC522_Wr(FIFODATAREG,sendData[i]);
    }

    MFRC522_Wr(COMMANDREG,command);

    if(command == PCD_TRANSCEIVE)
    {
        MFRC522_Set_Bit(BITFRAMINGREG,0x80);
    }

    i = 0xff;

    do
    {
        n = MFRC522_Rd(COMMIRQREG);
        i--;
    }while(i && !(n & 0x01) && !(n & waitIRq));

    MFRC522_Clear_Bit(BITFRAMINGREG,0x80);

    if(i!= 0)
    {
        if(!(MFRC522_Rd(ERRORREG)&0x1b))
        {
            _status = MI_OK;
            if(n & irqEn & 0x01)
            {
                _status = MI_NOTAGERR;
            }
            if(command == PCD_TRANSCEIVE)
            {
                n = MFRC522_Rd(FIFOLEVELREG);
                lastBits = MFRC522_Rd(CONTROLREG) & 0x07;
                if(lastBits)
                {
                    *backlen = (n-1)*8 + lastBits;
                }
                else
                {
                    *backlen = n*8;
                }
                if(n == 0)
                {
                    n = 1;
                }
                if(n>16)
                {
                    n = 16;
                }

                for(i =0;i<n;i++)
                {
                    backData[i] = MFRC522_Rd(FIFODATAREG);
                }
                backData[i] = 0;
            }
        }
        else
        {
            _status = MI_ERR;
        }
    }
    return _status;
}

char MFRC522_Request(char reqMode, char* TagType)
{
    char _status;
    unsigned char backBits;
    MFRC522_Wr(BITFRAMINGREG, 0x07);
    TagType[0] = reqMode;
    _status = MFRC522_ToCard(PCD_TRANSCEIVE,TagType,1,TagType,&backBits);
    if((_status != MI_OK) || (backBits != 0x10))
    {
        _status = MI_ERR;
    }
		return _status;
}

void MFRC522_CRC(char* dataIn, char length, char* dataOut)
{
    char i,n;
    MFRC522_Clear_Bit(DIVIRQREG,0x04);
    MFRC522_Set_Bit(FIFOLEVELREG, 0x80);

    for(i = 0;i<length;i++)
    {
        MFRC522_Wr(FIFODATAREG,dataIn[i]);
    }

    MFRC522_Wr(COMMANDREG, PCD_CALCCRC);

    i = 0xff;

    do
    {
       n = MFRC522_Rd(DIVIRQREG);
       i--;
    } while (i && !(n & 0x04));

    dataOut[0] = MFRC522_Rd(CRCRESULTREGL);
    dataOut[1] = MFRC522_Rd(CRCRESULTREGM);
}

char MFRC522_SelectTag(char* serNum)
{
    char i;
    char _status;
    char size;
    unsigned char recvBits;
    char buffer[9];
    buffer[0] = PICC_SElECTTAG;
    buffer[1] = 0x70;

    for(i=2;i<7;i++)
    {
        buffer[i] = *serNum++;
    }
    MFRC522_CRC(buffer,7,&buffer[7]);

    _status = MFRC522_ToCard(PCD_TRANSCEIVE,buffer,9,buffer,&recvBits);

    if((_status == MI_OK) && (recvBits == 0x18))
    {
        size = buffer[0];
    }
    else
    {
        size = 0;
    }
    return size;
}

void MFRC522_Halt()
{
    unsigned char unLen;
    char buff[4];

    buff[0] = PICC_HALT;
    buff[1] = 0;
    MFRC522_CRC(buff, 2, &buff[2]);
    MFRC522_Clear_Bit(STATUS2REG,0x80);
    MFRC522_ToCard(PCD_TRANSCEIVE,buff,4,buff,&unLen);
    MFRC522_Clear_Bit(STATUS2REG,0x80);
}

char MFRC522_Auth(char authMode, char BlockAddr, char* Sectorkey, char* serNum)
{
    char _status;
    unsigned char recvBits;
    char i;
    char buff[12];

    buff[0] = authMode;
    buff[1] = BlockAddr;

    for(i=2;i<8;i++)
    {
        buff[i] = Sectorkey[i-2];
    }

    for(i = 8;i<12;i++)
    {
        buff[i] = serNum[i-8];
    }
    _status = MFRC522_ToCard(PCD_AUTHENT,buff,12,buff,&recvBits);

    if((_status != MI_OK) || !(MFRC522_Rd(STATUS2REG) & 0x08))
    {
        _status = MI_ERR;
    }
    return _status;
}

char MFRC522_Write(char blockAddr, char* writeData)
{
    char _status;
    unsigned char recvBits;
    char i;
    char buff[18];
    buff[0] = PICC_WRITE;
    buff[1] = blockAddr;

    MFRC522_CRC(buff,2,&buff[2]);
    _status = MFRC522_ToCard(PCD_TRANSCEIVE,buff,4,buff,&recvBits);
    if((_status != MI_OK)|| (recvBits != 4) || ((buff[0]) & 0x0f!=0x0a))
    {
        _status = MI_ERR;
    }
    if(_status == MI_OK)
    {
        for(i = 0;i<16;i++)
        {
            buff[i] = writeData[i];
        }
        MFRC522_CRC(buff,16,&buff[16]);
        _status = MFRC522_ToCard(PCD_TRANSCEIVE,buff,18,buff,&recvBits);
        if((_status != MI_OK) || (recvBits !=4) || (buff[0] & 0x0f) != 0x0a)
        {
            _status = MI_ERR;
        }
    }
    return _status;
}

char MFRC522_Read(char blockAddr,char* recvData)
{
    char _status;
    unsigned char unLen;
    recvData[0] = PICC_READ;
    recvData[1] = blockAddr;

    MFRC522_CRC(recvData,2,&recvData[2]);
    _status = MFRC522_ToCard(PCD_TRANSCEIVE,recvData,4,recvData,&unLen);
    if ( (_status != MI_OK) || (unLen != 0x90) )
    {
        _status = MI_ERR;
    }
    return _status;
}

char MFRC522_Anticoll(char *serNum)
{
    char _status;
    char i;
    char serNumCheck = 0;
    unsigned char unlen;
    MFRC522_Wr(BITFRAMINGREG,0x00);
    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;
    MFRC522_Clear_Bit(STATUS2REG,0x80);
    _status = MFRC522_ToCard(PCD_TRANSCEIVE,serNum,2,serNum,&unlen);
    if (_status == MI_OK)
    {
        for ( i=0; i < 4; i++ )
        {
            serNumCheck ^= serNum[i];
        }
    
        if( serNumCheck != serNum[4] )
        {
            _status = MI_ERR;
        }
    }
  return _status;
}

char MFRC522_isCard( char *TagType ) 
{
    if (MFRC522_Request( PICC_REQIDL, TagType ) == MI_OK)
        return 1;
    else
        return 0; 
}

char MFRC522_ReadCardSerial( char *str )
{
    char _status; 
    _status = MFRC522_Anticoll( str );
    str[5] = 0;
    if (_status == MI_OK)
        return 1;
    else
        return 0;
}

