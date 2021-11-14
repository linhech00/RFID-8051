#include "REGX52.H"

//lcd_pin
sbit lcd_rs    			= P2^0;          
sbit lcd_rw         = P2^1;                                              
sbit lcd_en     		= P2^2;                        
sbit lcd_d4         = P2^4;                                                        
sbit lcd_d5         = P2^5;                                 
sbit lcd_d6         = P2^6;                                                  
sbit lcd_d7         = P2^7;

//rfid_pin
sbit MFRC522_CS       = P3^0;              
sbit MFRC522_SCK       = P3^1;
sbit MFRC522_SI        = P3^2;                          
sbit MFRC522_SO        = P3^3;            
sbit MFRC522_RST       = P3^4;

#include "rfidlib.h"

int i;
unsigned char g_ucTempbuf[20];
unsigned char _status;
unsigned char tembuf[5];
char count = 0; tmp = 0;

//---------------------------delay-----------------------------
//micro s
void delay_us(unsigned int Time)
{
	unsigned int i = Time;
	while(i--);
} 
//mini s
void delay_ms(unsigned int Time)
{
	unsigned int i,j;
	for(i=0;i<Time;i++)
	{
		for(j=0;j<125;j++);
	}
}

//-----------LCD---------------------
void lcd_kichxung()
{
	lcd_en = 1;
	delay_us(5);
	lcd_en = 0;
	delay_us(50);
}
//dat 4 bit du lieu vao 4 chan
void lcd_gui4bit(unsigned char dulieu)
{
	lcd_d4 = dulieu & 0x01;
	lcd_d5 = (dulieu>>1) & 0x01;
	lcd_d6 = (dulieu>>2) & 0x01;
	lcd_d7 = (dulieu>>3) & 0x01;
}
//gui day du 1 byte du lieu
void lcd_gui1byte(unsigned char dulieu)
{
	lcd_gui4bit(dulieu>>4); //gui 4 bit cao cua byte du lieu
	lcd_kichxung();
	lcd_gui4bit(dulieu);
	lcd_kichxung();	
}
//xoa hoan toan man hinh
void lcd_xoamanhinh()
{
	lcd_gui1byte(0x01);
	delay_ms(5);
}
//thiet lap man hinh lcd 
void lcd_caidat()
{
	lcd_gui4bit(0x00);
	delay_ms(50);
	lcd_rs =0;
	lcd_rw =0;
	lcd_gui4bit(0x03);
	lcd_kichxung();
	delay_ms(5);
	lcd_kichxung();
	delay_ms(100);
	lcd_kichxung();
	lcd_gui4bit(0x02);
	lcd_kichxung();
	lcd_gui1byte(0x28);
	lcd_gui1byte(0x0e);
	lcd_gui1byte(0x06);
	lcd_gui1byte(0x01);
	delay_ms(2);
}
//di chuyen con tro theo toa do 
void lcd_toadoxy(unsigned char x, unsigned char y)
{
	if(y) lcd_gui1byte(0xc0+x);
	else  lcd_gui1byte(0x80+x);
	delay_ms(1);
}
//gui 1 ki tu ra man hinh
void lcd_guikitu(unsigned char dulieu)
{
	lcd_rs = 1;
	lcd_gui1byte(dulieu);
	lcd_rs = 0;
}
//gui 1 chuoi ra man hinh
void lcd_guichuoi(char *str)
{
	while(*str)
	{
		lcd_guikitu(*str);
		str++;
	}
}

//char sosanh(unsigned char* dl , unsigned char* uid)
//{
//	for(i =0;i<5;i++)
//	{
//		if(dl[i]!=uid[i])
//			return 0;
//	}
//	return 1;
//}

void main()
{
	char uid[6];
	unsigned char TagType;
	lcd_caidat();
	MFRC522_Init();
	lcd_xoamanhinh();
	lcd_toadoxy(0,0);
	lcd_guichuoi("QUET UID MIFARE");
	lcd_toadoxy(0,1);
	lcd_guichuoi("CHO XIU NHA BB");
	delay_ms(2000);	
	lcd_xoamanhinh();
	delay_ms(1000);
	lcd_toadoxy(0,0);
	lcd_guichuoi("HIEN UID MIFIRE");
	lcd_toadoxy(0,1);
	lcd_guichuoi("UID: ");	
	delay_ms(2000);

	while (1)
	{
		lcd_toadoxy(7,1);
		if(MFRC522_isCard(&TagType))
		{
			if(MFRC522_ReadCardSerial(uid))
			{
				for(i=0;i<6;i++)
				{
					lcd_guikitu(uid[i]);
				}
			}
			MFRC522_Halt();
		}
		
	}
}



