/* standalone moon phase calculation */ 

#include <stdio.h>
#include <math.h>

#define     PI	            3.1415926535897932384626433832795
#define     RAD	         (PI/180.0)
#define     SMALL_FLOAT    (1e-12)
#define     YEARDAYS       365.25

typedef struct {
    int year, month, day;
    double hour;
} TimePlace;


void JulianToDate(TimePlace *now, double jd)
{
long jdi, b;
long a, c, d, e, g, g1;

   jd  = jd + 0.5;
   jdi = (long ) jd;

   b = jdi;
   if (jdi > 2299160)
   {
      a = (long) ( (jdi - 1867216.25) / 36524.25 );
      b = jdi + 1 + a - a/4;
   }
   
   c  = b + 1524;
   d  = (long) ( (c - 122.1)/YEARDAYS );
   e  = (long) ( YEARDAYS * d );
   g  = (long) ( (c - e)/30.6001 );
   g1 = (long) ( 30.6001 * g );
    
   now->day  = ( c - e - g1 );
   now->hour = ( jd - jdi) * 24.0;
    
   if( g <= 13 )
   {
      now->month = g - 1;
   }
   else
   {
      now->month = g - 13;
   }

   if( now->month > 2 )
   {
      now->year = d - 4716;
   }
   else
   {
      now->year = d - 4715;
   }
}


/*
    Returns the number of julian days for the specified day.
    */
double Julian(int year, int month, double day)
{
int a, b, c, e;

   if (month < 3)
   {
	   year  = year - 1;
	   month = month + 12;
   }

   if( ( (year >  1582)                                ) || 
       ( (year == 1582) && (month >  10)               ) ||
	    ( (year == 1582) && (month == 10) && (day > 15) ) 
     )
   {
	   a = year / 100;
	   b = 2 - a + a/4;
   }

   c = (long) (YEARDAYS * year);

   e = (long) (30.6001 * (month+1) );
   
   return b + c + e + day + 1720994.5;
}



double SunPosition(double j)
{
double n,x,e,l,dl,v;
int i;

   n = 360 / 365.2422 * j;
   i = (int) n / 360;
   n = n - i * 360.0;
   x = n - 3.762863;

   if( x < 0 )
   {
      x = x + 360.0;
   }

   x = x * RAD;
   e = x;
   
   do
   {
      dl = (double) (e - .016718 * sin(e) - x);
      e  = (double) (e - dl / (1-.016718 * cos(e) ) );
   }
   while (fabs(dl)>=SMALL_FLOAT);

   v = 360 / PI * atan(1.01686011182 * tan(e/2) );
   l = v + 282.596403;
   i = (int) (l / 360.0);
   l = l - i * 360;
   
   return l;
}

double MoonPosition(double j, double ls)
{
double ms, l, mm, n, ev, sms, ae, ec;
int    i;
    
    ms = 0.985647332099*j - 3.762863;
    if (ms < 0)
    {
       ms = ms + 360.0;
    }

    l = 13.176396*j + 64.975464;
    i = (int) (l/360.0);
    l = l - i * 360.0;
    
    if (l < 0)
    {
       l = l + 360.0;
    }
    
    mm  = l-0.1114041*j-349.383063;
    i   = (int)(mm/360.0);
    mm  = mm - i * 360.0;
    n   = 151.950429 - 0.0529539 * j;
    i   = (int)(n/360.0);
    n   = n - i * 360.0;
    ev  = 1.2739*sin((2*(l-ls)-mm)*RAD);
    sms = sin(ms*RAD);
    ae  = 0.1858*sms;
    mm  = mm + ev-ae- 0.37*sms;
    ec  = 6.2886*sin(mm*RAD);
    l   = l + ev + ec - ae + 0.214 * sin(2 * mm * RAD);
    l   = 0.6583 * sin(2 * (l-ls) * RAD) + l;
    
    return l;
}


/*
Calculates more accurately than Moon_phase , the phase of the moon at
the given epoch.
returns the moon phase as a real number (0-1)
*/
double MoonPhase(int year, int month, int day, double hour, int *ip)
{
double j    = 0.0;
double ls   = 0.0;
double lm   = 0.0;
double t    = 0.0;

   j  = Julian(year,month,(double)day+hour/24.0)-2444238.5;
   ls = SunPosition(j);
   lm = MoonPosition(j, ls);
   t  = lm - ls;

   if (t < 0)
   {
      t = t + 360.0;
   }
   
   *ip = (int)((t + 22.5)/45) & 0x7;
   
   return (1.0 - cos((lm - ls) * RAD)) / 2.0;
}

void NextDay(int *y, int *m, int *d, double dd)
{
TimePlace   tp;
double      jd = 0.0;

   jd = Julian(*y, *m, (double)*d);

   jd = jd + dd;
   JulianToDate(&tp, jd);

   *y = tp.year;
   *m = tp.month;
   *d = tp.day;
}


int main(void)
{
int y, m, d;
int m0;
int h;
int step = 1;
int begun = 0;

double pmax = 0;
double pmin = 1;
int ymax, mmax, dmax, hmax;
int ymin, mmin, dmin, hmin;
double plast = 0;

   printf("tabulation of the phase of the moon for one month\n\n");

   printf("year: "); fflush(stdout);
   scanf("%d", &y);

   printf("month: "); fflush(stdout);
   scanf("%d", &m);    

   d = 1;
   m0 = m;

   printf("\nDate       Time   Phase Segment\n");
   for (;;)
   {
      double p;
      int ip;
  
      for (h = 0; h < 24; h = h + step)
      {
         p = MoonPhase(y, m, d, h, &ip);
         if (begun)
         {
            if (p > plast && p > pmax)
            {
               pmax = p;
               ymax = y;
               mmax = m;
               dmax = d;
               hmax = h;
            }
            else if (pmax)
            {
               printf("%04d/%02d/%02d %02d:00          (fullest)\n",
                        ymax, mmax, dmax, hmax);
               pmax = 0;
             }
             if( (p < plast) && (p < pmin) )
             {
               pmin = p;
               ymin = y;
               mmin = m;
               dmin = d;
               hmin = h;
            }
            else if (pmin < 1)
            {
               printf("%04d/%02d/%02d %02d:00          (newest)\n",
                        ymin, mmin, dmin, hmin);
               pmin = 1.0;
            }
            if (h == 16)
            {
               printf("%04d/%02d/%02d %02d:00 %5.1f%%   (%d)\n",
                        y, m, d, h, floor(p*1000+0.5)/10, ip);
            }
         }
         else
         {
            begun = 1;
         }
         plast = p;
      }        
      NextDay(&y, &m, &d, 1.0);
      if (m != m0)
      {
         break;
      }
   }
   return 0;
}

