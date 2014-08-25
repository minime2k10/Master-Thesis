#include "StdAfx.h"
#include<opencv\cv.h>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include<opencv\highgui.h>
#include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include "Square.h"
#include <windows.h>


using namespace cv;
using namespace std;

char* stdNames[6] = { "orig.png", "binary.png","greyBlur.png","greyBlurBinary.png", "greyBinaryBlur.png", "grey.png"};

void updatelog(char* text, FILE* output);


int main(int argc, char *argv[])
	//char* sourceDir, char* outputDir)
{
	char sourceDir[512],outputDir[512];
	sourceDir[0] ='\0';
	outputDir[0] ='\0';
	strcpy(sourceDir,argv[1]);
	strcpy(outputDir,argv[2]);
	strcat(sourceDir,"*\0");
	WIN32_FIND_DATAA ffd;
	HANDLE hFind = FindFirstFileA(sourceDir, &ffd);
	
	//create a log file
	char outputLogText[1024];
	FILE* outputLog;
	outputLogText[0] = '\0';
	sprintf(outputLogText,"%sboxesTest.txt",outputDir);
	outputLog = fopen(outputLogText, "w");

	updatelog("Start of log",outputLog);
	
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
	
	//we now have a list of file names in an array
	for (int k = 0;k < filecounter;k++)
	{
		//create a new folder for each file with the different versions of file
		char newFolder[512], filePath[512];
		newFolder[0] = '\0';
		filePath[0] ='\0';
		sprintf(newFolder, "%s%s\\",outputDir, names[k]);
		sprintf(filePath, "%s%s",argv[1], names[k]);
		int newFolderLen = strlen(newFolder);
		CreateDirectoryA(newFolder, NULL);
		IplImage* temp;
		temp = cvLoadImage( filePath, 1 );
		strcat(newFolder,stdNames[0]);
		cvSaveImage(newFolder,temp);

		//create grayscale image
		IplImage* tempGrey = cvCreateImage(cvGetSize(temp),IPL_DEPTH_8U,1);
		cvCvtColor(temp,tempGrey,CV_RGB2GRAY);
		newFolder[newFolderLen] = '\0';
		strcat(newFolder,stdNames[5]);
		cvSaveImage(newFolder,tempGrey);

		updatelog(names[k],outputLog);
		updatelog("\n",outputLog);

		updatelog("Creating Binary\n",outputLog);

		//create binary version
		newFolder[newFolderLen] = '\0';
		strcat(newFolder,stdNames[1]);
		IplImage *temp2;
		temp2 = cvLoadImage(filePath,CV_LOAD_IMAGE_GRAYSCALE);

		IplImage *binary = cvCloneImage( temp2 );
		cvThreshold( temp2, binary, 225, 255,0 );
		cvSaveImage(newFolder,binary);

		updatelog("Finished Binary\n",outputLog);

		updatelog("Creating  Blur\n",outputLog);
		//now bilateral blur image
		cvReleaseImage( &temp2 );
		temp2 = cvCloneImage(binary);
		cvSmooth(binary, temp2, CV_BILATERAL, 5,5,100,100); 
		newFolder[newFolderLen] = '\0';
		strcat(newFolder,stdNames[4]);
		cvSaveImage(newFolder,temp2);

		//free up temp versions
		cvReleaseImage( &temp2 );
		cvReleaseImage( &binary );

		
		updatelog("Creating Guassian Blur\n",outputLog);
		
		//Bi-Lateral blur version
		temp2 = cvLoadImage(filePath,CV_LOAD_IMAGE_GRAYSCALE);
		IplImage *guassian = cvCloneImage(temp2);
		cvSmooth(temp2, guassian, CV_BILATERAL, 5,5,90,90); 
		newFolder[newFolderLen] = '\0';
		strcat(newFolder,stdNames[2]);
		cvSaveImage(newFolder,guassian);
		cvReleaseImage( &temp2 );
		temp2 = cvCloneImage(guassian);

		updatelog("Finished Guassian Blur\n",outputLog);

		updatelog("Creating Binary\n",outputLog);
		
		//threshold above image
		cvThreshold( guassian, temp2, 200, 255,0 );
		newFolder[newFolderLen] = '\0';
		strcat(newFolder,stdNames[3]);
		cvSaveImage(newFolder,temp2);
		cvReleaseImage( &temp2 );
		cvReleaseImage( &guassian );

		updatelog("Finished Binary\n",outputLog);

		//reset foldername
		newFolder[newFolderLen] = '\0';


		updatelog("Find Squares\n",outputLog);
		//run findsquares for images
		squareMain(stdNames,6,newFolder,newFolder);

		updatelog("Finished Squares\n",outputLog);
	}

	sourceDir[0] ='\0';
	strcpy(sourceDir,argv[1]);
	//squareMain(names,filecounter,outputDir,sourceDir);
}

void updatelog(char* text, FILE* output)
{
	fwrite(text,strlen(text),1,output);
	fflush(output);
}
