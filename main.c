// https://pro-diod.ru/electronica/max7219-max7221-drajver-dlya-svetodiodnoj-indikacii.html

#include "reg51.h"
#include "intrins.h"
#include "mathlib.h"
#include "main.h"

typedef unsigned char byte;
#define ESPI 0x02
#define FOSC 11059200UL
#define BRT (65536 - FOSC / 115200 / 4)

sfr     AUXR    =   0x8e;
sfr     T2H     =   0xd6;
sfr     T2L     =   0xd7;

sfr 		SPSTAT 	= 	0xcd;
sfr 		SPCTL 	= 	0xce;
sfr 		SPDAT 	= 	0xcf;
sfr 		IE2 		= 	0xaf;
sfr 		P_SW1 	= 	0xa2;

sfr     P0M1    =   0x93;
sfr     P0M0    =   0x94;
sfr     P1M1    =   0x91;
sfr     P1M0    =   0x92;
sfr     P2M1    =   0x95;
sfr     P2M0    =   0x96;
sfr     P3M1    =   0xb1;
sfr     P3M0    =   0xb2;
sfr     P4M1    =   0xb3;
sfr     P4M0    =   0xb4;
sfr     P5M1    =   0xc9;
sfr     P5M0    =   0xca;
sfr     P4      =   0xc0;
sfr 		P5 			= 	0xC8;

sbit 		SS 			= 	P3^5;

sfr			RSTCFG	= 	0xFF; // reset config register

bit 		busy;
char wptr;
char rptr;
char buffer[16];

byte pointFlag;
byte eFlag;
byte func;
byte minus;


double reg;
double mreg;
double x, y;
byte pointSet, memSet;
byte scan[5] = {0xEF, 0xF7, 0xFB, 0xFD, 0xFE};

// UATR
void UartIsr() interrupt 4
{
    if (TI)
    {
        TI = 0;
        busy = 0;
    }
    if (RI)
    {
        RI = 0;
        buffer[wptr++] = SBUF;
        wptr &= 0x0f;
    }
}

void UartInit()
{
    SCON = 0x50;
    T2L = BRT;
    T2H = BRT >> 8;
    AUXR = 0x15;
    wptr = 0x00;
    rptr = 0x00;
    busy = 0;
}

void UartSend(char dat)
{
    while (busy);
    busy = 1;
    SBUF = dat;
}

void UartSendStr(char *p)
{
    while (*p)
    {
        UartSend(*p++);
    }
}

void UartPrintLong(long l){
	char snum[10];
	ltoa(l, snum);
	UartSendStr(snum);
	UartSendStr("\r\n");
}

void UartPrintDbl(double d){
	char res[9];
	dtoa(d, res);
	UartSendStr(res);
	UartSendStr("\r\n");
}

// SPI

void SPI_Isr() interrupt 9 
{
	SPSTAT = 0xc0; //Clear interrupt flag	
	busy = 0;	
}

void Send(byte addr, byte dataSet){
	while (busy);
	busy = 1;	
	SS = 0;
	SPDAT = addr; //Send data hi
	while (busy);
	SPDAT = dataSet; //Send data low
	busy = 1;
	while (busy);
	SS = 1;
	
}

void DisplayInit(){
	Send(0x0F, 0x00);
	Send(0x0C, 0x01);
	Send(0x0B, 0x07);
	Send(0x09, 0x00);
	Send(0x0A, 0x08); // bright
}

void DidplayClear(){
	int i;
	for(i = 2; i < 9; i++){
		Send(i, 0);
	}
	Send(1, 0x80); // set . 
}

void Display(int i, byte b, byte point){
	switch(b){
		case 0xFD: // - and 'memory set'
			Send(8, 0x01 | point);
			return;
		case 0xFE: // E
			Send(8, 0x4F);
			return;
		case 0xFF: // clear place
			Send(i, 0x00 | point);
			return;
		case 0x00:
			Send(i, 0x7E | point);
			return;
		case 0x01:
			Send(i, 0x30 | point);
			return;
		case 0x02:
			Send(i, 0x6D | point);
			return;
		case 0x05:
			Send(i, 0x5B | point);
			return;
		case 0x08:
			Send(i, 0x7F | point);
			return;
		case 0x04:
			Send(i, 0x33 | point);
			return;
		case 0x07:
			Send(i, 0x70 | point);
			return;
		case 0x03:
			Send(i, 0x79 | point);
			return;
		case 0x06:
			Send(i, 0x5F | point);
			return;
		case 0x09:
			Send(i, 0x7B | point);
			return;
	}
}

byte Decode(byte col, byte row){
	switch(row){
		case 0:
			switch(col){
				case 0xFE:
					return 0x19; // C
				case 0xFD:
					return 0x01; // 1
				case 0xEF:
					return 0x04; // 4
				case 0xDF:
					return 0x07; // 7
				default:
				break;
			}
		case 1: 
			switch(col){
				case 0xFE:
					return 0x00; // 0
				case 0xFD:
					return 0x02; // 2
				case 0xEF:
					return 0x05; // 5
				case 0xDF:
					return 0x08; // 8
				default:
				break;
		}
		case 2: 
			switch(col){
				case 0xFE:
					return 0x0A; // .
				case 0xFD:
					return 0x03; // 3
				case 0xEF:
					return 0x06; // 6
				case 0xDF:
					return 0x09; // 9
				default:
				break;
		}
		case 3: 
			switch(col){
				case 0xFE:
					return 0x11; // +
				case 0xFD:
					return 0x12; // -
				case 0xEF:
					return 0x13; // *
				case 0xDF:
					return 0x14; // /
				default:
				break;
		}
		case 4: 
			switch(col){
				case 0xFE:
					return 0x15; // =
				case 0xFD:
					return 0x16; // MC
				case 0xEF:
					return 0x17; // MR
				case 0xDF:
					return 0x18; // M+
				default:
				break;
		}	
	}
	return 0xFF;
}

int GetLenght(long i){
	int l;
	l = 1;
	while(i / 10 > 0){
		i /= 10;
		l++;
	}
	return l;
}

void Show3(long reg, byte point, byte min){
	long pos, r;
	byte f;
	int i;
	byte p, flag;
	f = 0xFF;
	if(min) f = 0xFD;
	Display(8, f, memSet);
	pos = 1000000;
	flag = 0;
	for(i = 7; i > 0; i--){
		r = reg / pos;
		p = 0;
		if(flag == 0) flag = r;
		if(i == point) flag = 1;
		if(flag > 0 || i == 1) {
			if(i == point) p = 0x80;
			Display(i, r, p);
		}
		else Display(i, 0xFF, p);
		reg -= (r * pos);
		pos /= 10;
	}
}

void Delay500ms(void)	//@11.0592MHz
{
	unsigned char data i, j, k;

	_nop_();
	_nop_();
	i = 22;
	j = 3;
	k = 227;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

double GetValue(){
	double t;
	if(! pointSet) {
		t = reg;
	} else t = reg / (pow(10, pointSet - 1));
	if(minus) t = 0 - t;
	return t;
}

void SetErr(){
	eFlag = 1;
}

void Calc(){
	double t;
	int i, j, m;
	switch(func){
		case 0x11:
			x = x + y;
			break;
		case 0x12:
			x = x - y;
			break;
		case 0x13:
			x = x * y;
			break;
		case 0x14:
			x = x / y;
			break;
	}
	if(x > 9999999 || x < -9999999){
		SetErr();
		return;
	}
	if(x < 0) minus = 1; 
		else minus = 0;
	t = fabs(x);
	reg = (long)t;
	pointSet = 0;
	i = GetLenght(reg);
	if(t != (long)t){
		t = t - reg;
		for(j = i+1; j < 8; j++){
			t = t * 10;
			m = (int) t;
			reg = (reg * 10) + m;
			t = t - (long)t;
		}
		pointSet = 8 - i;
		for(i = 1; i < 7; i++){
			t = (double) reg;
			if((long)(t / 10) * 10 != t) break;
			reg = (long)(reg / 10);
			pointSet--;
		}
	}
}

void GetReg(double xr){
	double t;
	int i, j, m;
	if(xr < 0) minus = 1; 
	else minus = 0;
	t = fabs(xr);
	reg = (long)t;
	pointSet = 0;
	i = GetLenght(reg);
	if(t != (long)t){
		t = t - reg;
		for(j = i+1; j < 8; j++){
			t = t * 10;
			m = (int) t;
			reg = (reg * 10) + m;
			t = t - (long)t;
		}
		pointSet = 8 - i;
		for(i = 1; i < 7; i++){
			t = (double) reg;
			if((long)(t / 10) * 10 != t) break;
			reg = (long)(reg / 10);
			pointSet--;
		}
	}
}

void Reset(){
	DisplayInit();
	DidplayClear();
		
	P2 = 0xFF;
	reg = 0;
	x = 0;
	y = 0;
	pointFlag = 0;
	eFlag = 0;
	pointSet = 0;
	func = 0;
	minus = 0;
	Show3(0, 0, 0);
}

void main()
{
	  byte key, read, readFlag, point, inputX, cx;
		int row;
		double var;	
	
    P0M0 = 0x00;
    P0M1 = 0x00;
    P1M0 = 0x00;
    P1M1 = 0x00;
    P2M0 = 0x00;
    P2M1 = 0x00;
    P3M0 = 0x00;
    P3M1 = 0x00;
    P4M0 = 0x00;
    P4M1 = 0xff;
    P5M0 = 0x00;
    P5M1 = 0x00;
	
		RSTCFG = 0x50; // reset config: low volt & P5.4
	  
	  SPCTL = 0xD0; 	//Enable SPI master mode
	  SPSTAT = 0xc0; 	//Clear interrupt flag
	  IE2 = ESPI; 		//Enable SPI interrupt
	  P_SW1 = 0x0c; 	//SS_4/P3.5, MOSI_4/P3.4, MISO_4/P3.3, SCLK_4/P3.2
		UartInit();
    ES = 1;
    EA = 1;
	  busy = 0;
	
	  DisplayInit();
		DidplayClear();
		
		memSet = 0;
		mreg = 0;
		readFlag = 1;
		point = 1;
		inputX = 1;
		cx = 0;
		Reset();
	
		UartSendStr("Run calculator\r\n");
		UartPrintDbl(12.45);
		
	  while (1)
    {
			// check keys
			P1 = 0xFF;
			read = 0xFF;
			for(row = 0; row < 5; row++){
				P2 = scan[row];
				if(P1 != 0xFF){
					read = P1;
					break;
				}
			}
			P2 = 0xFF;
			if(read != 0xFF){
				if(readFlag && !eFlag){
					key = Decode(read, row);
					if(key < 0x0A && pointSet < 7){
						if(cx){ // begin input y
							reg = 0;
							cx = 0;
							minus = 0;
							pointSet = 0;
						}
						reg *= 10;
						reg += key;
						if(pointSet){
							pointSet++;
						}
						if(reg > 9999999){
							eFlag = 1;
						} 
					}else if(key == 0x0A && !pointSet) {// .
					  pointSet = 1;
					}else if(key == 0x11 || key == 0x12 || key == 0x13 || key == 0x14){ // + - * /
						func = key;
						if(inputX){
							x = GetValue();
							inputX = 0;
							cx = 1;
						}else{
							y = GetValue();
							inputX = 1;
							Calc();
						}
					} else if(key == 0x15){ // =
						y = GetValue();
						inputX = 1;
						Calc();
					} else if(key == 0x18){ // M+
						mreg = GetValue();
						memSet = 0x80;
					} else if(key == 0x16){ // MC
						memSet = 0;
						mreg = 0;
					} else if(key == 0x17 && memSet > 0){ // MR
						GetReg(mreg);
					} else if(key == 0x19){ // C
						Reset();
						readFlag = 1;
						point = 1;
						inputX = 1;
						cx = 0;
					}
					
					var =(double) reg / pow(10, pointSet);
					Show3(reg, pointSet, minus);
					read = 0xFF;
					readFlag = 0;
					Delay500ms();
				}
			}else{
				readFlag = 1;
			}
    }
	
}