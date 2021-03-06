#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "jpeglib.h"
#include "SLIC.h"
#include "opencv_lbp.h"
#include <limits>
#include <iostream>
#include <fstream>
#include <sstream>
#include "time.h"
#include "math.h"

using namespace cv;
using namespace std;

#define JPEG_QUALITY 90
#define LBP_VECTORS 16
 
void elbp(Mat& src, Mat &dst, int radius, int neighbors);

int main(int argc, char* argv[])
{
	clock_t start, finish;

	float a[21],b[21];
    int histcount = 0;

	//-----------------------------------
	// 01: read jpg to buffer[char]
	//-----------------------------------
	start = clock();
	Mat raw_image = imread("test.jpg" , 1);
	if(raw_image.empty())
		printf("imread failed!\n");
//	else 
//		printf("%d, %d.\n", a.rows, a.cols);
	//imshow("test", a);
	//waitKey(0);	
	
	int h = raw_image.rows;
	int w = raw_image.cols;
	int i(0), j(0);
	int x(0), y(0);	//pixel:x,y

	unsigned char * image_buffer = (unsigned char *)malloc(sizeof(char) * w * h * 4)  ; // BGRA  
    unsigned int  * pbuff        = (unsigned int  *)malloc(sizeof(int) * w * h);
	
	for(i = 0; i < h; i++)
	{	
		for(j = 0; j < w; j++)
		{	
			*(image_buffer + i * w * 4 + j*4+0 ) = raw_image.at<Vec3b>(i,j).val[0];		//B
			*(image_buffer + i * w * 4 + j*4+1 ) = raw_image.at<Vec3b>(i,j).val[1];		//G
			*(image_buffer + i * w * 4 + j*4+2 ) = raw_image.at<Vec3b>(i,j).val[2];		//R
			*(image_buffer + i * w * 4 + j*4+3 ) = 0;									//A=0
		}
	}	
	finish = clock();
	printf("01 read jpeg to buffer[char]: %lf.\n", (double)(finish-start)/CLOCKS_PER_SEC);
		
	//----------------------------------
    // 02: SLIC Initialize parameters
    //----------------------------------
    int k = 2000;		//Desired number of superpixels.
	int k_real = 0;	 	//Truth number of superpixel, k_real>k generally.
    double m = 10;		//Compactness factor. use a value ranging from 10 to 40 depending on your needs. Default is 10
    int* klabels = (int *)malloc(sizeof(int) * w * h);		//lable map
    int numlabels(0);
    string filename = "temp.jpg";		//seg image
    string savepath = "/home/xduser/LiHuan/SLICO/example/result/";

	//---------------------------------------------
	// 03: transform buffer[char] to buffer[int]
	//---------------------------------------------	
    // unsigned int (32 bits) to hold a pixel in ARGB format as follows:
    // from left to right,
    // the first 8 bits are for the alpha channel (and are ignored)
    // the next 8 bits are for the red channel
    // the next 8 bits are for the green channel
    // the last 8 bits are for the blue channel
	start = clock();
	for(i = j = 0; i < w*h; i++,j+=4 )
	{	
		*(pbuff + i) = *(image_buffer+j+3) + \
					  (*(image_buffer+j+2))	* 256 + \
					  (*(image_buffer+j+1)) * 256 * 256 + \
				      (*(image_buffer+j+0)) * 256 * 256 * 256;
	}
	for(i=0; i<w*h*4; i++)
	{
		*(image_buffer+i)=0;
	}
    finish = clock();
	printf("02 transform buffer[char] to [int]: %lf.\n", (double)(finish-start)/CLOCKS_PER_SEC);
	
	//-----------------------------------------
    // 04: Perform SLIC on the image buffer
    //-----------------------------------------
	start = clock();
    SLIC segment;
    segment.PerformSLICO_ForGivenK(pbuff, w, h, klabels, numlabels, k, m);
    // Alternately one can also use the function PerformSLICO_ForGivenStepSize() for a desired superpixel size

    //---------------------------------------
    // 05: Save the labels to a text file
    //---------------------------------------
	//segment.SaveSuperpixelLabels(klabels, w, h, filename, savepath);
	
	//--------------------------------------------
	// 06: find the number of real superpixels
	//--------------------------------------------
    for(i = 0; i < w*h; i++)
    {
        if(klabels[i] > k_real)
            k_real = klabels[i];
    }
    printf("k_real = %d\n",k_real);
	finish = clock();
	printf("06 SLIC on buffer: %lf.\n", (double)(finish-start)/CLOCKS_PER_SEC);

	//------------------------------------------	
	// 07: put pixel cluster into vector
	//------------------------------------------
	start = clock();
	vector<vector<int> > superpixel;	// 0 ~ n superpixel
	vector<int> i_superpixel;			// the pixel:x,y in the ith superpixel 

	for(i = 0; i < k_real + 1; i++)
	{	
		for(j = 0; j < w * h; j++)
		{
			if(klabels[j] == i)
			{	
				y = j % w ;
				x = (j - y ) / w;
				i_superpixel.push_back(x);
				i_superpixel.push_back(y);
			}
		}
		superpixel.push_back(i_superpixel);	
		i_superpixel.clear();
	}
	printf("Read into vector ok!\n");
	finish = clock();
	printf("07 put into vector: %lf.\n", (double)(finish-start)/CLOCKS_PER_SEC);

#if 0
	for(vector<vector<int> >::size_type ix = 0; ix < superpixel.size(); ix++) 	//superpixel label number
		for(vector<int>::size_type iy = 0; iy < superpixel[1000].size(); iy += 2)	//get all x,y pixel in ith super
			printf("(%d, %d)",superpixel[1000][iy], superpixel[1000][iy+1]);
	printf("\n");	
#endif

#if 0	
	int n20,n30,n40,n50,n60,n70,n80,n90,n100,n110,n120,n130,n140;
	n20 = n30 = n40 = n50 = n60 = n70 = n80 = n90 = n100 = n110 = n120 = n130 = n140 = 0;
	for(vector<vector<int> >::size_type ix = 0; ix < superpixel.size(); ix++) 	//superpixel label number
	{
		int p = int(superpixel[ix].size()) / 2;
		if( p>=20 && p<30 ) n20++;
		if( p>=30 && p<40 ) n30++;
		if( p>=40 && p<50 ) n40++;
		if( p>=50 && p<60 ) n50++;
		if( p>=60 && p<70 ) n60++;
		if( p>=70 && p<80 ) n70++;
		if( p>=80 && p<90 ) n80++;
		if( p>=90 && p<100 ) n90++;
		if( p>=100 && p<110 ) n100++;
		if( p>=110 && p<120 ) n110++;
		if( p>=120 && p<130 ) n120++;
		if( p>=130 && p<140 ) n130++;
		if( p>=140 && p<200 ) n140++;
	}
	printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", n20,n30,n40,n50,n60,n70,n80,n90,n100,n110,n120,n130,n140);
#endif

	//-------------------------------------
	// 08: Extract Feature 
	//-------------------------------------
	printf("Feature Extracting ...\n");
	// LBP feature[16]  LABXY[5]
	vector<vector<float> > lbp_superpixel;
	vector<vector<float> > labxy_superpixel;
	vector<float> temp;
	int max_x(0), min_x(65536), max_y(0), min_y(65536);
	int color_l(0), color_a(0), color_b(0); 

    Mat image_lbp = imread("test.jpg", 0);		// 0:grayscale model
	printf("image_lbp H: %d, W: %d\n", image_lbp.rows, image_lbp.cols);
	if(image_lbp.empty())
		printf("imread fail.\n");

	Mat image_lab = imread("test.jpg", 1);		// BGR color model
	printf("image_lab H: %d, W: %d\n", image_lab.rows, image_lab.cols);
	if(image_lab.empty())
		printf("imread fail.\n");

	Mat img_lbp; 
	Mat img_lab;
	int radius(1), neighbors(8);
	Mat lbp_feature, lab_feature;	
	MatND hist;
	int bins = LBP_VECTORS;     //default = 16
	int hist_size[] = {bins};   
	float range[]   = {0, 256};
	const float * ranges[] = {range};
	int channels[] = {0};
	int sizeofrect;
	
	//   
	start = clock();
	for(vector<vector<int> >::size_type ix = 0; ix < superpixel.size(); ix++) 	//superpixel label number
	{
		max_x = max_y = 0;
		min_x = min_y = 65536;
		color_l = color_a = color_b = 0;
		for(vector<int>::size_type iy = 0; iy < superpixel[ix].size(); iy += 2)	//get all x,y pixel in ith super
		{	
			if(superpixel[ix][iy  ] > max_x)	max_x = superpixel[ix][iy];
			if(superpixel[ix][iy+1] > max_y)	max_y = superpixel[ix][iy+1];
			if(superpixel[ix][iy]   < min_x)	min_x = superpixel[ix][iy];
			if(superpixel[ix][iy+1] < min_y)	min_y = superpixel[ix][iy+1];
		}
		Rect rect(min_y, min_x, max_y-min_y+1, max_x-min_x+1);	//(1,3)>rect(3,1)
		sizeofrect = (max_y-min_y+1) * (max_x-min_x+1);

//		printf("%d-", sizeofrect);
//		if(int(ix)  == 1000)
//			printf("%d,%d,%d,%d\n",min_y,min_x,max_y,max_x);
//			printf("rect : %d,%d,%d,%d\n", min_y, min_x, max_y-min_y+1, max_x-min_x+1);	//(1,3)>rect(3,1)
//		if(int(ix)  == 1)
//			printf("%d,%d,%d,%d\n",min_y,min_x,max_y,max_x);
		// lbp feature
   	 	img_lbp = image_lbp(rect);
//		if(int(ix)  == 1)
//			printf("\n%d, %d\n", img_lbp.rows, img_lbp.cols);
    	lbp_feature = Mat(img_lbp.rows-2*radius, img_lbp.cols-2*radius,CV_8UC1, Scalar(0));
   	 	elbp(img_lbp, lbp_feature, 1, 8);
 
	   	calcHist(&lbp_feature, 1, channels, Mat(), hist, 1, hist_size, ranges, true, false);
		for(i=0; i < bins; i++)
			temp.push_back( hist.at<float>(i) );
		lbp_superpixel.push_back(temp);
		temp.clear();    	
		
		// LAB,xy feature	
		img_lab = image_lab(rect);    	//imshow("image",img);
		cvtColor(img_lab, lab_feature, COLOR_BGR2Lab);
		for(i=0; i<(max_x-min_x+1); i++)
		{	
			for(j=0; j<(max_y-min_y+1); j++)
			{
				color_l += (int)lab_feature.at<Vec3b>(i,j).val[0];				
				color_a += (int)lab_feature.at<Vec3b>(i,j).val[1];				
				color_b += (int)lab_feature.at<Vec3b>(i,j).val[2];				
			}
		}
		temp.push_back( (float)color_l / sizeofrect );
		temp.push_back( (float)color_a / sizeofrect );
		temp.push_back( (float)color_b / sizeofrect );
		temp.push_back( (max_x-min_x+1)/2 + min_x );
		temp.push_back( (max_y-min_y+1)/2 + min_y );
		labxy_superpixel.push_back(temp);
		temp.clear();    	
	}//for 0~2016
	printf("Feature get ! \n");
	finish = clock();	
	printf("for loop needs: %lf.\n", (double)(finish-start)/CLOCKS_PER_SEC);
	
	printf("lbp_superpixel   = %d.\n", int(lbp_superpixel.size()));
	printf("labxy_superpixel = %d.\n", int(labxy_superpixel.size()));

#define first 835
#define second 1100

	// 对两个块求特征，归一化，然后计算相似性(越小越好)

	printf("[");
	for(vector<int>::size_type ix = 0, i=0; ix < lbp_superpixel[first].size(), i<16; ix++,i++) 	//superpixel label number
	{
		a[i] = lbp_superpixel[first][ix]; 
		histcount +=  a[i];
		printf("%3d", int(lbp_superpixel[first][ix]) );
	}
	for(i=0; i<16; i++)
        a[i] /= histcount;
    histcount = 0;

	printf("\t");
	for(vector<int>::size_type ix = 0, i=16; ix < labxy_superpixel[first].size(), i<21; ix++,i++) 	//superpixel label number
	{
		a[i] = labxy_superpixel[first][ix];
		printf("%5d", int(labxy_superpixel[first][ix]) );
	}
	printf("]\n");
	for(i=16; i<19; i++)
		a[i] /= 255;
	a[19] /= h;
	a[20] /= w;	


    printf("\n[");
    for(vector<int>::size_type ix = 0, i=0; ix < lbp_superpixel[second].size(), i<16; ix++,i++)  //superpixel label number
    {
        b[i] = lbp_superpixel[second][ix];
        histcount +=  b[i];
        printf("%3d", int(lbp_superpixel[second][ix]) );
    }
    for(i=0; i<16; i++)
        b[i] /= histcount;
    histcount = 0;

    printf("\t");
    for(vector<int>::size_type ix = 0, i=16; ix < labxy_superpixel[second].size(), i<21; ix++,i++)   //superpixel label number
    {
        b[i] = labxy_superpixel[second][ix];
        printf("%5d", int(labxy_superpixel[second][ix]) );
    }
    printf("]\n");
    for(i=16; i<19; i++)
        b[i] /= 255;
    b[19] /= h;
    b[20] /= w;

	printf("\nAfter normalized.\n");
    for(i=0; i<21; i++)
        printf("%f,", a[i]);
	printf("\n");
    for(i=0; i<21; i++)
        printf("%f,", b[i]);
	printf("\n");

	float sum1 = 0;
	float sum2 = 0;
	float sum3 = 0;
	for(i=0; i<16; i++)
		sum1 += pow( (a[i]-b[i]), 2);
	for(i=16; i<19; i++)
		sum2 += pow( (a[i]-b[i]), 2);
	for(i=19; i<21; i++)
		sum3 += pow( (a[i]-b[i]), 2);
	printf("\nDistance = %f.\n", (0.7 * sqrt(sum1) + 0.2 * sqrt(sum2) + 0.1 * sqrt(sum3)) );
/*	
*/
/*
	//----------------------------------
    // Draw boundaries around segments
    //----------------------------------
	printf("\n06. draw contour to buffer.\n");
    segment.DrawContoursAroundSegments(pbuff, klabels, w, h, 0xff0000);
	printf("06 ok.\n");

    //----------------------------------
    // Save the image with segment boundaries.
    //----------------------------------
	for(i = j = 0; i < w*h; i++,j+=3 )
    {
        *(image_buffer+j+2) = ( (*(pbuff + i)) & 0xff);
        *(image_buffer+j+1) = ( ( (*(pbuff + i)) >> 8) & 0xff);
        *(image_buffer+j+0) = ( ( (*(pbuff + i)) >> 16) & 0xff);
    } 
    write_jpeg_file("/home/xduser/LiHuan/SLICO/example/result/temp_seg.jpg", image_buffer, w, h, JPEG_QUALITY);
    printf("savejpeg ok!\n");

*/	
    //----------------------------------
    // Clean up
    //----------------------------------
	free(klabels);
	free(image_buffer);
	free(pbuff);

	return 0;


}

