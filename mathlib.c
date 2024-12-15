#include "mathlib.h"
#include "string.h"


void reverse(char s[])
 {
     int i, j;
     char c;
 
     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
 }

 void ltoa(long n, char s[])
 {
     long i, sign;
 
     if ((sign = n) < 0)  
         n = -n;          
     i = 0;
     do {       
         s[i++] = n % 10 + '0';   
     } while ((n /= 10) > 0);    
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
 }
 
 // Converts a given integer x to string str[]. 
// d is the number of digits required in the output. 
// If d is more than the number of digits in x, 
// then 0s are added at the beginning. 
int longToStr(long x, char str[]) 
{ 
    int i = 0; 
    while (x) { 
        str[i++] = (x % 10) + '0'; 
        x = x / 10; 
    } 
    reverse(str); 
    str[i] = '\0'; 
    return i; 
} 
 
 // Converts a floating-point/double number to a string. 
void dtoa(double n, char* res) 
{ 
    // Extract integer part 
    long ipart = (long)n; 
 
    // Extract floating part 
    double fpart = n - (double)ipart; 
 
    // convert integer part to string 
    int i = longToStr(ipart, res); 
 
    // check for display option after point 
    res[i] = '.'; // add dot 
		fpart *= 10;
		res += i; 
	  while((long)fpart){
			res++;
			longToStr((long)fpart, res); 
			fpart -= (long)fpart; 
			fpart *= 10;
		}
}