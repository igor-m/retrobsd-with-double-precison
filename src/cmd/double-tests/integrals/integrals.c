// RETROBSD double floating point example
// Integrals test
// Based on http://rosettacode.org/wiki/Numerical_integration
// Pito 4/2014

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Integration methods 
//
double int_leftrect(double from, double to, int n, double (*func)())
{
   double h = (to-from)/n;
   double sum = 0.0, x;
   for(x=from; x <= (to-h); x += h)
      sum += func(x);
   return h*sum;
}
 
double int_rightrect(double from, double to, int n, double (*func)())
{
   double h = (to-from)/n;
   double sum = 0.0, x;
   for(x=from; x <= (to-h); x += h)
     sum += func(x+h);
   return h*sum;
}
 
double int_midrect(double from, double to, int n, double (*func)())
{
   double h = (to-from)/n;
   double sum = 0.0, x;
   for(x=from; x <= (to-h); x += h)
     sum += func(x+h/2.0);
   return h*sum;
}
 
double int_trapezium(double from, double to, int n, double (*func)())
{
   double h = (to - from) / n;
   double sum = func(from) + func(to);
   int i;
   for(i = 1;i < n;i++)
       sum += 2.0*func(from + i * h);
   return  h * sum / 2.0;
}
 
double int_simpson(double from, double to, int n, double (*func)())
{
   double h = (to - from) / n;
   double sum1 = 0.0;
   double sum2 = 0.0;
   int i;
 
   double x;
 
   for(i = 0;i < n;i++)
      sum1 += func(from + h * i + h / 2.0);
 
   for(i = 1;i < n;i++)
      sum2 += func(from + h * i);
 
   return h / 6.0 * (func(from) + func(to) + 4.0 * sum1 + 2.0 * sum2);
}

/* test functions */
// F1
double f1(double x)
{
  return (   x*x /(  (-1.0 + pow(x,4.0)) * (-1.0 + pow(x,4.0)) )  );
}
 
double f1a(double x)
{
  return ( ( (-4.0 * x*x*x)/(-1.0 + x*x*x*x) - 2.0*atan(x) - log(-1.0 + x) + log(1.0 + x))/16.0 );
}

// F2
double f2(double x)
{
  return ( sin(x + 1.0)*cos(x - 1.0)/tan(x - 1.0) );
}
 
double f2a(double x)
{
  return ( ( 2.0*x*cos(2.0) + 4.0*log(sin(1.0 - x))*sin(2.0) + sin(2.0*x) )/4.0  ) ;
}

// F3
double f3(double x)
{
  return ( (5.0 + 3.0*x)/(1.0 - x - x*x + x*x*x) );
}
 
double f3a(double x)
{
  return ( -4.0/(-1.0 + x) - log(-1.0 + x)/2.0 + log(1.0 + x)/2.0 ) ;
}


// F4
double f4(double x)
{
  return ( 1.0 /(2.0 + x*x*x*x) );
}
 
double f4a(double x)
{
	double k = 2.0;
	double m = 0.25;
  return ( (-k*atan(1.0 - pow(k,m)*x) + k*atan(1.0 + \
  		pow(k,m)*x) - log(k - k*pow(k,m)*x + sqrt(k)*x*x) + \
  		log(k + k*pow(k,m)*x + sqrt(k)*x*x))/(8.0*pow(k,m))   ) ;
} 

  
typedef double (*pfunc)(double, double, int , double (*)());
typedef double (*rfunc)(double);
 
#define INTG(F,A,B) (F((B))-F((A)))
 
int main()
{
     int i, j;
     double ic,  tmp;
 
     pfunc f[5] = { 
       int_leftrect, int_rightrect,
       int_midrect,  int_trapezium,
       int_simpson 
     };

     const char *names[5] = {
       "leftrect", "rightrect", "midrect",
       "trapezium", "simpson" 
     };

     rfunc rf[] = { f1, f2, f3, f4 };
     rfunc If[] = { f1a, f2a, f3a, f4a };

     double ivals[] = { 
       2.0, 10.0,
       -1.0, -0.7,
       10.0, 50.0,
       0.0, 10.0
     };
     
     int approx[] = { 10000, 10000, 10000, 10000 };
 
     for(j=0; j < (sizeof(rf) / sizeof(rfunc)); j++)
     {
       for(i=0; i < 5 ; i++)
       {
         ic = (*f[i])(ivals[2*j], ivals[2*j+1], approx[j], rf[j]);
         tmp = INTG((*If[j]), ivals[2*j], ivals[2*j+1]);
         printf("%10s [%1.1e %1.1e] num: %1.11e, ana: %1.11e\n",
            names[i], ivals[2*j], ivals[2*j+1], ic, tmp);
       }
       printf("\n");
     }
}