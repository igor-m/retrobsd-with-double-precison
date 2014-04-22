// RETROBSD double floating point example
// 9 degree test
// Pito 4/2014

#include <stdio.h>
#include <math.h>
#include <sys/time.h>

int main() {

	volatile double _p64, q64, r64;
	volatile float _p32, q32, r32;
	long i;
	unsigned long elapsed;
	
	_p64 = 3.1415926535897932384626433832795;
	_p32 = 3.1415926535897932384626433832795;

	// 64bit test
	// 9 degree test input
	elapsed = msec();
	for (i=1; i<=1000; i++) {
	q64 = 9.0;
	// Convert to radians
	q64 = q64 * _p64 / 180.0;
	// Make the test	
	r64 = (asin(acos(atan(tan(cos(sin(q64)))))));
	// Convert to degree
	r64 = r64 * 180.0 / _p64;
	}
	elapsed = msec() - elapsed;
	
	printf("9degree 64bit test result= %1.15f  time= %lu usecs\r\n", r64, elapsed );

	// 32bit test
	// 9 degree test input
	elapsed = msec();
	for (i=1; i<=1000; i++) {
	q32 = 9.0;
	// Convert to radians
	q32 = q32 * _p32 / 180.0;
	// Make the test	
	r32 = (asin(acos(atan(tan(cos(sin(q32)))))));
	// Convert to degree
	r32 = r32 * 180.0 / _p32;
	}
	elapsed = msec() - elapsed;
	
	printf("9degree 32bit test result= %1.6f  time= %lu usecs\r\n", r32, elapsed );

	
	return 0;
}