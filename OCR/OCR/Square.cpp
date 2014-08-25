#include "stdafx.h"
#include <opencv\cv.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/core/core.hpp>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <Windows.h>


using namespace cv;
using namespace std;

int sqrThresh = 50;
IplImage* img = 0;
IplImage* img0 = 0;
CvMemStorage* storage = 0;
CvPoint pt[4];

long long ImageArea, ImageWidth, ImageHeight;

char* glbOutputOrig, *glbOutput;

struct boxesRec
{
	int count;
	int* points[1024];
};

boxesRec box;

void addBox(int x1, int y1, int x2, int y2);
void removeDuplicates();

// helper function:
// finds a cosine of angle between vectors
// from pt0->pt1 and from pt0->pt2
double angle( CvPoint* pt1, CvPoint* pt2, CvPoint* pt0 )
{
    double dx1 = pt1->x - pt0->x;
    double dy1 = pt1->y - pt0->y;
    double dx2 = pt2->x - pt0->x;
    double dy2 = pt2->y - pt0->y;
    return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

// returns sequence of squares detected on the image.
// the sequence is stored in the specified memory storage
CvSeq* findSquares4( IplImage* img, CvMemStorage* storage )
{
    CvSeq* contours;
    int i, c, l, N = 11;
    CvSize sz = cvSize( img->width & -2, img->height & -2 );
    IplImage* timg = cvCloneImage( img ); // make a copy of input image
    IplImage* gray = cvCreateImage( sz, 8, 1 ); 
    IplImage* pyr = cvCreateImage( cvSize(sz.width/2, sz.height/2), 8, 3 );
    IplImage* tgray;
    CvSeq* result;
    double s, t;
    // create empty sequence that will contain points -
    // 4 points per square (the square's vertices)
    CvSeq* squares = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvPoint), storage );
   
    // select the maximum ROI in the image
    // with the width and height divisible by 2
    cvSetImageROI( timg, cvRect( 0, 0, sz.width, sz.height ));
   
    // down-scale and upscale the image to filter out the noise
    cvPyrDown( timg, pyr, 7 );
    cvPyrUp( pyr, timg, 7 );
    tgray = cvCreateImage( sz, 8, 1 );
   
    // find squares in every color plane of the image
    for( c = 0; c < 3; c++ )
    {
        // extract the c-th color plane
        cvSetImageCOI( timg, c+1 );
        cvCopy( timg, tgray, 0 );
       
        // try several threshold levels
        for( l = 0; l < N; l++ )
        {
            // hack: use Canny instead of zero threshold level.
            // Canny helps to catch squares with gradient shading   
            if( l == 0 )
            {
                // apply Canny. Take the upper threshold from slider
                // and set the lower to 0 (which forces edges merging)
                cvCanny( tgray, gray, 0, sqrThresh, 5 );
                // dilate canny output to remove potential
                // holes between edge segments
                cvDilate( gray, gray, 0, 1 );
            }
            else
            {
                // apply threshold if l!=0:
                //     tgray(x,y) = gray(x,y) < (l+1)*255/N ? 255 : 0
                cvThreshold( tgray, gray, (l+1)*255/N, 255, CV_THRESH_BINARY );
            }
           
            // find contours and store them all as a list
            cvFindContours( gray, storage, &contours, sizeof(CvContour),
                CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE );
           
            // test each contour
            while( contours )
            {
                // approximate contour with accuracy proportional
                // to the contour perimeter
                result = cvApproxPoly( contours, sizeof(CvContour), storage,
                    CV_POLY_APPROX_DP, cvContourPerimeter(contours)*0.02, 0 );
                // square contours should have 4 vertices after approximation
                // relatively large area (to filter out noisy contours)
                // and be convex.
                // Note: absolute value of an area is used because
                // area may be positive or negative - in accordance with the
                // contour orientation
                if( result->total == 4 &&
                    fabs(cvContourArea(result,CV_WHOLE_SEQ)) > 1000 &&
                    cvCheckContourConvexity(result) )
                {
                    s = 0;
                   
                    for( i = 0; i < 5; i++ )
                    {
                        // find minimum angle between joint
                        // edges (maximum of cosine)
                        if( i >= 2 )
                        {
                            t = fabs(angle(
                            (CvPoint*)cvGetSeqElem( result, i ),
                            (CvPoint*)cvGetSeqElem( result, i-2 ),
                            (CvPoint*)cvGetSeqElem( result, i-1 )));
                            s = s > t ? s : t;
                        }
                    }
                   
                    // if cosines of all angles are small
                    // (all angles are ~90 degree) then write quandrange
                    // vertices to resultant sequence
                    if( s < 0.3 )
                        for( i = 0; i < 4; i++ )
                            cvSeqPush( squares,
                                (CvPoint*)cvGetSeqElem( result, i ));
                }
               
                // take the next contour
                contours = contours->h_next;
            }
        }
    }
   
    // release all the temporary images
    cvReleaseImage( &gray );
    cvReleaseImage( &pyr );
    cvReleaseImage( &tgray );
    cvReleaseImage( &timg );
   
    return squares;
}


// the function draws all the squares in the image
void drawSquares( IplImage* img, CvSeq* squares )
{
	//for testing

	FILE* outputTest;
	char text[512];
	text[0] = '\0';
	sprintf(text,"%sboxesTest.txt",glbOutput);
	outputTest = fopen(text, "w");


	//end of testing
    CvSeqReader reader;
    IplImage* cpy = cvCloneImage( img );
    int i;
	ImageArea = cpy->width * cpy->height;
	ImageWidth = cpy->width;
	ImageHeight = cpy->height;

    // initialize reader of the sequence
    cvStartReadSeq( squares, &reader, 0 );
    
    // read 4 sequence elements at a time (all vertices of a square)
    for( i = 0; i < squares->total; i += 4 )
    {
        CvPoint* rect = pt;
        int count = 4;
        
        // read 4 vertices
        memcpy( pt, reader.ptr, squares->elem_size );
        CV_NEXT_SEQ_ELEM( squares->elem_size, reader );
        memcpy( pt + 1, reader.ptr, squares->elem_size );
        CV_NEXT_SEQ_ELEM( squares->elem_size, reader );
        memcpy( pt + 2, reader.ptr, squares->elem_size );
        CV_NEXT_SEQ_ELEM( squares->elem_size, reader );
        memcpy( pt + 3, reader.ptr, squares->elem_size );
        CV_NEXT_SEQ_ELEM( squares->elem_size, reader );
		//ignore whole picture boxes
		int wholeBox = 0;
		for (int n = 0; n<4;n++)
		{
			if (pt[n].x <= 5 && pt[n].y <= 5)
			{
				wholeBox = 1;
			}
		}

		if (wholeBox == 0)
		{
			unsigned int minX = -1, minY = -1,maxX = 0,maxY = 0;
			for (int k = 0;k<4;k++)
			{
				if (pt[k].x < minX)
				{
					minX = pt[k].x;
				}
				if(pt[k].y < minY)
				{
					minY = pt[k].y;
				}
				if(pt[k].x > maxX)
				{
					maxX = pt[k].x;
				}
				if(pt[k].y > maxY)
				{
					maxY = pt[k].y;
				}
			} 
			//adjust to get area around image
			minX = minX -10;
			minY = minY -10;
			maxX = maxX + 10;
			maxY = maxY + 10;
			char lineOut[64];

			//cvPolyLine( cpy, &rect, &count, 1, 1, CV_RGB(0,255,0), 3, 8 );
			sprintf(lineOut,"%u,%u,%u,%u\n\r\0",minX,minY,maxX-minX,maxY-minY);
			int size = strlen(lineOut);
			fwrite(lineOut, size,1,outputTest);
			//add to boxes
			addBox(minX,minY,maxX,maxY);
		}
    }
	fclose(outputTest);

	//remove duplicate entries
	removeDuplicates();

	FILE* output;
	char textpath[512];
	textpath[0] = '\0';
	sprintf(textpath,"%sboxes.txt",glbOutput);
	output = fopen(textpath, "w");
	int fileCounter = 0;
	for (int z = 0;z< box.count;z++)
	{
		//additional wdith/height check!
		if (!(box.points[z][3] > 50 && box.points[z][3] < 100 && box.points[z][2] > 500 && box.points[z][2] < 1000)  && !(box.points[z][3] > 20 && box.points[z][3] < 55 && box.points[z][2] > 100 && box.points[z][2] < 450))
		{
			//do nothing
			int x;
			x=0;
		}
		else if (!(box.points[z][0] == 0 && box.points[z][1] == 0 && box.points[z][2] == 0 && box.points[z][3] == 0))
		{
			Rect roi(box.points[z][0], box.points[z][1], box.points[z][2], box.points[z][3]);
			CvPoint* rect = new CvPoint[4];
			rect[0].x = box.points[z][0];
			rect[0].y = box.points[z][1];
			rect[1].x = box.points[z][0];
			rect[1].y = box.points[z][1] + box.points[z][3];
			rect[3].x = box.points[z][0] + box.points[z][2];
			rect[3].y = box.points[z][1];
			rect[2].x = box.points[z][0] + box.points[z][2];
			rect[2].y = box.points[z][1] + box.points[z][3];
			int countPts = 4;
			// draw the square as a closed polyline
			//cvPolyLine( cpy, &rect, &countPts, 1, 1, CV_RGB(0,255,0), 3, 8 );
			delete[] rect;
			//need to export box from here
			IplImage* cimg = cvCloneImage( cpy );
			cvSetImageROI(cimg, roi);
			char path[512];
			sprintf(path,"%sfile%i.jpg",glbOutput,fileCounter++);
			cvSaveImage( path, cimg);
			//cvShowImage("subimage",cimg);
			//got image in box now, export
			cvReleaseImage(&cimg);
			char line[32];
			sprintf(line,"%d,%d,%d,%d\n\r\0",box.points[z][0],box.points[z][1],box.points[z][2],box.points[z][3]);
			int size = strlen(line);
			fwrite(line, size,1,output);
		}
	}
	fclose(output);
   
    // show the resultant image
    cvShowImage("image",cpy);
    cvReleaseImage( &cpy );
}

void removeDuplicates()
{
	//need to remove any duplicates
	for (int i = 0;i< box.count;i++)
	{
		int own = i;
		int x = box.points[i][0];
		int y = box.points[i][1];
		int width = box.points[i][2];
		int length = box.points[i][3];
		for (int j = 0; j < box.count;j++)
		{
			//check its not own record
			if (j != own)
			{
				if (x == box.points[j][0] && y == box.points[j][1] && width == box.points[j][2] && length == box.points[j][3])
				{
					//blank out record
					box.points[j][0] = 0;
					box.points[j][1] = 0;
					box.points[j][2] = 0;
					box.points[j][3] = 0;
				}
			}
		}
	}
}

void addBox(int x1, int y1, int x2, int y2)
{
	//this code if for boxes on - experiment 3 - can be commented out otherwise
	//check width between 500 and 1000 and height between 50 and 100
	int cmpWidth = x2 - x1;
	int cmpHeight = y2 - y1;
	if (!(cmpWidth < 1000 && cmpWidth > 500 && cmpHeight > 50 && cmpHeight < 100) && !(cmpWidth < 450 && cmpWidth > 100 && cmpHeight > 20 && cmpHeight < 55))
	{
		//not in ranges
		return;
	}
	//end of experiment 3 code

	int exists = 0;
	//need to check box is not larger than 40% of area of picture
	if ((x2 - x1) * (y2 - y1) > (ImageArea / 4))
	{
		exists = 1;
		return;
	}
	//check points exist within image
	if (x1 > ImageWidth || x2 > ImageWidth || y1 > ImageHeight || y2 > ImageHeight)
	{
		exists = 1;
		return;
	}

	//check not existing box
	for (int i = 0;i< box.count;i++)
	{
		if ((box.points[i][0] >= x1 - 10 && box.points[i][0] <= x1 + 10) && (box.points[i][1] >= y1 - 10 && box.points[i][1] <= y1 + 10))
		{
			//both x and y pixels in 10 pixel block... take largest box
			exists = 1;
			if (x1 < box.points[i][0])
			{
				box.points[i][0] = x1; 
			}
			if (y1 < box.points[i][1])
			{
				box.points[i][1] = y1; 
			}
			if (x2 > box.points[i][2] + box.points[i][0])
			{
				box.points[i][2] = x2 - box.points[i][0]; 
			}
			if (y2 > box.points[i][3] + box.points[i][1])
			{
				box.points[i][3] = y2 - box.points[i][1]; 
			}
		}
		//check its not a sub box
		else if ((box.points[i][0] <= x1 && box.points[i][0] + box.points[i][2]) >= x2 && (box.points[i][1] <= y1 && box.points[i][1] + box.points[i][3] >= y2))
		{
			//entire box is encapsulated in larger box
			exists = 1;
			if (x1 < box.points[i][1])
			{
				box.points[i][0] = x1; 
			}
			if (y1 < box.points[i][2])
			{
				box.points[i][1] = y1; 
			}
			if (x2 > box.points[i][2] + box.points[i][0])
			{
				box.points[i][2] = x2 - box.points[i][0]; 
			}
			if (y2 > box.points[i][3] + box.points[i][1])
			{
				box.points[i][3] = y2 - box.points[i][1]; 
			}
		}
		//does it encapsulate a box?
		else if ((x1 - 10 <= box.points[i][0] && x2 + 10 >= box.points[i][0] + box.points[i][2]) && (y1 - 10 <= box.points[i][1] && y2 + 10 >= box.points[i][1] + box.points[i][3]))
		{
			//yes it does!
			if (x2 - x1 > box.points[i][2])
			{
				box.points[i][2] = x2 - x1; 
			}
			if (y2 - y1 > box.points[i][3])
			{
				box.points[i][3] = y2 - y1; 
			}
			if (x1 < box.points[i][1])
			{
				box.points[i][0] = x1; 
			}
			if (y1 < box.points[i][2])
			{
				box.points[i][1] = y1; 
			}			
			exists = 1;
		}
	}
	if (exists == 0)
	{
		//new box!
		box.points[box.count] = new int[4];
		box.points[box.count][0] = x1;
		box.points[box.count][1] = y1;
		box.points[box.count][2] = x2 - x1;
		box.points[box.count][3] = y2 - y1;
		box.count++;
	}
}



void on_trackbar( int a )
{
    if( img )
        drawSquares( img, findSquares4( img, storage ) );
}


void squareMain(char** names,int noPics, char* outputPath, char* inputPath)
{
   int i;
   glbOutputOrig = outputPath;
   // create memory storage that will contain all the dynamic data
   storage = cvCreateMemStorage(0);

    for( i = 0; i < noPics; i++ )
    {
        // load i-th image
		char filePath[512];
		filePath[0] ='\0';
		sprintf(filePath,"%s%s",inputPath,names[i]);
        
		//load original image
		img0 = cvLoadImage( filePath, 1 );
        if( !img0 )
        {
            printf("Couldn't load %s\n", names[i] );
            break;
        }
		char newFolder[512];
		
		//create output path
		newFolder[0] = '\0';
		sprintf(newFolder, "%s%sOutput\\",outputPath, names[i]);
		CreateDirectoryA(newFolder, NULL);
		
		glbOutput = newFolder;

        //create a clone of original image for work
		img = cvCloneImage( img0 );
		
		//try find squares on original image
		drawSquares( img, findSquares4( img, storage ) );

		// release both images
        cvReleaseImage( &img );
        cvReleaseImage( &img0 );
        // clear memory storage - reset free space position
        cvClearMemStorage( storage );
    }
   
    cvDestroyWindow("image");
}