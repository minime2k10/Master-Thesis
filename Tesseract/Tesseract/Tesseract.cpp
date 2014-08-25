// Tesseract.cpp : Defines the entry point for the console application.
//

// Cplusplus1.cpp : Defines the entry point for the console application. 
// 

#include "stdafx.h" 
#include <ctype.h>
#include <Windows.h>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/core/core.hpp>
#include "opencv2/legacy/compat.hpp"
#include "opencv2\opencv.hpp"



using namespace cv; 


int high_switch_value = 1; 
int highInt = 12; 
int low_switch_value = 0; 
int lowInt = 0; 

/*void switch_callback_h(int position) { 
        highInt = position; 
}*/

double a = 0, b = 0; 
int contrast = 0, brightness = 0; 

void updateContrast(int _contrast) { 
        contrast = _contrast - 100; 

        if( contrast > 0 ) 
    { 
        double delta = 127.*contrast/100; 
        a = 255./(255. - delta*2); 
        
    } 
    else 
    { 
        double delta = -128.*contrast/100; 
        a = (256.-delta*2)/255.; 
        } 
} 

void updateBrightness(int _brightness) { 
        brightness = _brightness - 100; 
        
        if(contrast > 0) { 
                double delta = 127.*contrast/100; 
        b = a*(brightness - delta);	
        } 
        else { 
                double delta = -128.*contrast/100; 
                b = a*brightness + delta; 
        } 
} 


int OCR(char* filepath, char* outputpath, char* filename) 
{ 
        const char* name = "Edge Detection Window"; 
		char outputFile[2048];
        outputFile[0]= '\0';
		// Kernel size 
        int N = 1; 

		IplImage* im_gray, *img2;

        IplImage* img = cvLoadImage( filepath, 1 );
		
		//save a copy of original to output folder
		sprintf(outputFile,"%s%sorig.jpg",outputpath,filename);
		cvSaveImage( outputFile, img ); 
		outputFile[0]='\0';

		//create unsharpened binary version
		//convert to gray
		img2 = cvCloneImage(img);
		im_gray = cvCreateImage(cvGetSize(img2),IPL_DEPTH_8U,1);
		cvCvtColor(img2,im_gray,CV_RGB2GRAY);
		cvReleaseImage(&img2);
		
		//create binary sharpened version
		img2 = cvCloneImage(im_gray);
		cvThreshold( im_gray, img2, 200, 255,0 );
		sprintf(outputFile,"%s%sbinary.tif",outputpath,filename);
		cvSaveImage( outputFile, img2 ); 
		outputFile[0]='\0';
		cvReleaseImage(&img2);
		cvReleaseImage(&im_gray);


		IplImage* out = cvCreateImage( cvSize(img->width, img->height), IPL_DEPTH_8U, 3); 
        IplImage* contrast = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 3); 

		// optimization by unmasking only luminance channel 
        IplImage* luma = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 3); 
        IplImage* y = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1); 
        IplImage* rgb = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 3); 
        IplImage* unmask = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 3); 
        IplImage* temp_y = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1); 
        IplImage* cr = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1); 
        IplImage* cb = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_8U, 1); 
        cvCvtColor(img, luma, CV_BGR2YCrCb); 
        // y is luminance channel on what we will be doing unmask 
        int aperature_size = N; 
        double lowThresh = 20; 
        double highThresh = 40; 


        int P = 1; 
                       
        N = highInt; 
        P = lowInt;
		updateContrast(73);
		updateBrightness(123);
        
		// Edge Detection 
        cvCvtColor(img, luma, CV_BGR2YCrCb); 
        cvSplit(luma, y, cr, cb, 0); 
                        
        cvSmooth(y, temp_y, CV_GAUSSIAN, N*2 + 3, N*2 + 3, 0, 0); 
        cvMerge(temp_y, cr, cb, 0, luma); 
                        
        cvCvtColor(luma, rgb, CV_YCrCb2BGR); 
        cvSub(img, rgb, unmask, 0); 
                        
        cvConvertScale(img, contrast, a, b); 
                        
        cvAdd(contrast, unmask, out, 0); // add unmask and contrast images 
        cvAdd(img, out, out, 0);   // add  original with above one 

		//sharpening processing done.
		sprintf(outputFile,"%s%ssharpen.tif",outputpath,filename);
		cvSaveImage( outputFile, img ); 
		outputFile[0]='\0';
		
		//convert to gray
		img2 = cvCloneImage(img);
		im_gray = cvCreateImage(cvGetSize(img2),IPL_DEPTH_8U,1);
		cvCvtColor(img2,im_gray,CV_RGB2GRAY);
		cvReleaseImage(&img2);
		
		//create binary sharpened version
		img2 = cvCloneImage(im_gray);
		cvThreshold( im_gray, img2, 200, 255,0 );
		sprintf(outputFile,"%s%sbinarysharpen.tif",outputpath,filename);
		cvSaveImage( outputFile, img2 ); 
		outputFile[0]='\0';
		cvReleaseImage(&img2);
		cvReleaseImage(&im_gray);
        
		
		cvReleaseImage( &img ); 
        cvReleaseImage(&luma); 
        cvReleaseImage(&y); 
        cvReleaseImage(&cr); 
        
        cvReleaseImage( &out ); 
        cvDestroyWindow( name ); 

    return 0; 

} 

int main(int argc, char* argv[])
{
	char sourceDir[512],outputDir[512];
	sourceDir[0] ='\0';
	outputDir[0] ='\0';
	strcpy(sourceDir,argv[1]);
	strcpy(outputDir,argv[2]);
	strcat(sourceDir,"*\0");
	WIN32_FIND_DATAA ffd;
	HANDLE hFind = FindFirstFileA(sourceDir, &ffd);
	
	char** names;
	names = new char*[512];
	int filecounter = 0;
	if (INVALID_HANDLE_VALUE == hFind) 
	{
     return 1;
	} 

	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			//do nothing, not doing recursive yet!
		}
		else
		{
			names[filecounter] = new char[strlen(ffd.cFileName) + 1];
			names[filecounter][0] = '\0';
			strcpy(names[filecounter],ffd.cFileName);
			filecounter++;
		}
	}
	while (FindNextFileA(hFind, &ffd) != 0);
	int i;
	for (i = 0;i<filecounter;i++)
	{
		//create char arrays and set to null string
		char filename[1024];
		char name[256];
		name[0] ='\0';
		filename[0] = '\0';
		
		//create a copy of the name minus extension
		strcpy(name,names[i]);
		int namelength = strlen(names[i]);
		name[namelength - 4] = '\0';

		//create input filename
		sprintf(filename,"%s%s",argv[1],names[i]);
		
		//run OCR
		OCR(filename, outputDir,name);
	}

}
