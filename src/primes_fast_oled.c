/* primes_fast_oled
 * 
 * Print primes on OLED
 * Uses deterministic Miller-Rabin primality test:
 *  https://stackoverflow.com/questions/61188233/miller-rabin-deterministic-primality-test-c
 * Claims to work above 2^32 but it returns false for 4294967311.
 * Uses CH32V003FUN environmant
 * Alun Morris 110724
 * OLED code: 03-29-2023 E. Brombaugh
 * 
 * Version
 * 110724 Published
 */

//type of OLED
//#define SSD1306_64X32
#define SSD1306_128X32
//#define SSD1306_128X64

#include <stdio.h>								//there are printf()s in OLED lib for errors
#include "ch32v003fun.h"
#include "ssd1306_i2c.h"
#include "ssd1306.h"

//********************************** prototypes *********************/
int is_prime_mr(unsigned long long);
unsigned long long mul_mod(unsigned long long , unsigned long long , unsigned long long );
unsigned long long powermod(unsigned long long , unsigned long long , unsigned long long );
//********************************** main ***************************/
int main()
{
	char oledStr[33] = "";
	unsigned long number = 3000000001;              //start number <2^32. Must be odd.
	unsigned long  firstPrime = 0, primesFound = 0,  gapAverage = 0;

	SystemInit();
	funGpioInitAll();
	funPinMode( PC4, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP );
	funDigitalWrite( PC4, FUN_HIGH );

	Delay_Ms( 100 );

	// init i2c and oled
	Delay_Ms( 100 );	// give OLED some more time
	if(!ssd1306_i2c_init()) {
		ssd1306_init();
		ssd1306_drawstr(0,0, "Primes on OLED", 1);
		ssd1306_refresh();
		Delay_Ms(2000);
		funDigitalWrite( PC4, FUN_LOW );
		
		while(1) {
			if (is_prime_mr(number)) {
				if (primesFound == 0) {
					firstPrime = number;
					primesFound++;
				}
				else {							//>=2 found
					gapAverage = (number - firstPrime)/primesFound++;
				}
				funDigitalWrite( PC4, FUN_HIGH );							
				ssd1306_setbuf(0);
				sprintf(oledStr, "%lu", number);
				ssd1306_drawstr(0, 0, oledStr, 1);
				sprintf(oledStr, "%lu", primesFound);
				ssd1306_drawstr(0, 8, "Primes found=", 1);
				ssd1306_drawstr(0, 16, oledStr, 1);
				sprintf(oledStr, "%lu", gapAverage);
				ssd1306_drawstr(0, 24, "Av gap=", 1);
				ssd1306_drawstr(56, 24, oledStr, 1);
				ssd1306_refresh();
			}
			number += 2;	
				funDigitalWrite( PC4, FUN_LOW );							
		}		
	}
	while(1);			//failed if here
}

//**************************** functions *******************************/

//computes (a*b) (mod n)
unsigned long long mul_mod(unsigned long long a, unsigned long long b, unsigned long long m)
{
   unsigned long long d = 0, mp2 = m >> 1;
   if (a >= m) a %= m;
   if (b >= m) b %= m;
   for (int i = 0; i < 64; ++i) {
       d = (d > mp2) ? (d << 1) - m : d << 1;
       if (a & 0x8000000000000000ULL)
           d += b;
       if (d >= m) d -= m;
       a <<= 1;
   }
   return d;
}
//computes (a^b) (mod m)
unsigned long long powermod(unsigned long long a, unsigned long long b, unsigned long long m)
{
    unsigned long long r = m==1?0:1;
    while (b > 0) {
        if (b & 1)
            r = mul_mod(r, a, m);
        b = b >> 1;
        a = mul_mod(a, a, m);
    }
    return r;
}

// n−1 = 2^s * d with d odd by factoring powers of 2 from n−1
unsigned long long witness(unsigned long long n, unsigned long long s, unsigned long long d, unsigned long long a)
{
    unsigned long long x = powermod(a, d, n);
    unsigned long long y;

    while (s) {
        y = (x * x) % n;
        if (y == 1 && x != 1 && x != n-1)
            return 0;
        x = y;
        --s;
    }
    if (y != 1)
        return 0;
    return 1;
}

/*
 * if n < 1,373,653, it is enough to test a = 2 and 3;
 * if n < 9,080,191, it is enough to test a = 31 and 73;
 * if n < 4,759,123,141, it is enough to test a = 2, 7, and 61;
 * if n < 1,122,004,669,633, it is enough to test a = 2, 13, 23, and 1662803;
 * if n < 2,152,302,898,747, it is enough to test a = 2, 3, 5, 7, and 11;
 * if n < 3,474,749,660,383, it is enough to test a = 2, 3, 5, 7, 11, and 13;
 * if n < 341,550,071,728,321, it is enough to test a = 2, 3, 5, 7, 11, 13, and 17.
 */

int is_prime_mr(unsigned long long n)
{
    if (((!(n & 1)) && n != 2 ) || (n < 2) || (n % 3 == 0 && n != 3))
        return 0;
    if (n <= 3)
        return 1;

    unsigned long long d = n / 2;
    unsigned long long s = 1;
    while (!(d & 1)) {
        d /= 2;
        ++s;
    }
    unsigned long long a1 = 4759123141;
    unsigned long long a2 = 1122004669633;
    unsigned long long a3 = 2152302898747;
    unsigned long long a4 = 3474749660383;
    if (n < 1373653)
        return witness(n, s, d, 2) && witness(n, s, d, 3);
    if (n < 9080191)
        return witness(n, s, d, 31) && witness(n, s, d, 73);
    if (n < a1)
        return witness(n, s, d, 2) && witness(n, s, d, 7) && witness(n, s, d, 61);
    if (n < a2)
        return witness(n, s, d, 2) && witness(n, s, d, 13) && witness(n, s, d, 23) && witness(n, s, d, 1662803);
    if (n < a3)
        return witness(n, s, d, 2) && witness(n, s, d, 3) && witness(n, s, d, 5) && witness(n, s, d, 7) && witness(n, s, d, 11);
    if (n < a4)
        return witness(n, s, d, 2) && witness(n, s, d, 3) && witness(n, s, d, 5) && witness(n, s, d, 7) && witness(n, s, d, 11) && witness(n, s, d, 13);
    return witness(n, s, d, 2) && witness(n, s, d, 3) && witness(n, s, d, 5) && witness(n, s, d, 7) && witness(n, s, d, 11) && witness(n, s, d, 13) && witness(n, s, d, 17);
}
