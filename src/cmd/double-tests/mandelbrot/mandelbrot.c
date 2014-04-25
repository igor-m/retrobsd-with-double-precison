// Mandelbrot at Seahorse Valley
// Retrobsd torture test
// Pito 12/2013 under MIT License
// based on http://lodev.org/cgtutor/juliamandelbrot.html
// and bmp converter snippets found on the web
// no checks for proper parameters yet !
// usage ie.:
// Mandel  Xsize Ysize Iterations/pixel   zoom   moveX   moveY  filename
// Mandel  100 100 1024  1779.803945297549 -0.7431533999637661 -0.1394057861346605 seah.bmp
// Prints out a bmp picture!!! :)

#include "bmp.h"

int main(int argc, char **argv)
{ 
	if (argc >1){

		// size of the screen
		int w = atoi(argv[1]);   
		int h = atoi(argv[2]);

		// current screen coordinates
		int x, y; 

		// the colour value for the pixel
		int color; 

		// open bitmap image
		newbmp(w, h);

		// after how much iterations the function should stop
		int maxIterations = atoi(argv[3]); 

		// real and imaginary part of the pixel p
		double pr, pi;     

		// real and imaginary parts of new and old z
		double newRe, newIm, oldRe, oldIm;   

		// you may change these coordinates to zoom in and change 
		// the picture position within Mandelbrot
		// "Seahorse Valley" location:
		//double zoom = 1779.803945297549;
		//double moveX = -0.7431533999637661;
		//double moveY = -0.1394057861346605; 

		double zoom = atof(argv[4]);
		double moveX = atof(argv[5]);
		double moveY = atof(argv[6]);

		// loop through the screen
		for(x = 0; x < w; x++)
			for(y = 0; y < h; y++)
			{
				// calculate the initial real and imaginary part of z,
				// based on the pixel location and zoom and position values
				pr = 1.5 * (x - w / 2.0) / (0.5 * zoom * w) + moveX;
				pi = (y - h / 2.0) / (0.5 * zoom * h) + moveY;

				//these should start at 0,0
				newRe = newIm = oldRe = oldIm = 0.0; 			

				// "i" will represent the number of iterations
				int i;

				// start the iteration process
				for(i = 0; i < maxIterations; i++)
				{
					// remember value of previous iteration
					oldRe = newRe;
					oldIm = newIm;

					// the actual iteration, the real and imaginary part are calculated
					newRe = oldRe * oldRe - oldIm * oldIm + pr;
					newIm = 2.0 * oldRe * oldIm + pi;

					// if the point is outside the circle with radius 2: stop
					if((newRe * newRe + newIm * newIm) > 4.0) break;
				}

				color = i;

				//draw the pixel, retrobsd colours
				if (color < maxIterations / 3 )
				{
					putpixel(y, x, RGB( color%255 , 0 , 0) );
				}
				else if (( color > (maxIterations / 3) ) && (color < (2 * maxIterations / 3)) )
				{
					putpixel(y, x, RGB( 100 , color%255 , 0 ));
				}
				else 
				{
					putpixel(y, x, RGB( 200 , 250 , color%255 ));
				}
			}

		// save the bitmap
		savebmp(argv[7]);
		closebmp();

	}
	else
	{
		printf("Mandelbrot without parameters Xsize Ysize Iterations..\n\n");
	}

	return 0;
}


