/*#include <QtGui/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    
    return a.exec();
}
*/
#include <iostream>
#include <cmath>
#include <opencv/cv.h>
#include <opencv/highgui.h>

typedef float (*to_float_t)(void **data);

float from_32F_F(void **data)
{
    float f=**((float **)data);
    *((float **)data)=*((float **)data)+1;
    return f;
}
float from_32S_F(void **data)
{
    float f=**((int **)data);
    *((int **)data)=*((int **)data)+1;
    return f;
}
float from_8U_F(void **data)
{
    float f= (**((unsigned char **)data))/256.0f;
    *((char **)data)=*((char **)data)+1;
    return f;
}
float from_8S_F(void **data)
{
    float f=(**((char **)data) + 128.0f)/256.0f;
    *((char **)data)=*((char **)data)+1;
    return f;
}
float from_16U_F(void **data)
{
    float f=(**((unsigned short **)data))/65535.0f;
    *((short **)data)=*((short **)data)+1;
    return f;
}
float from_16S_F(void **data)
{
    float f= (**((short **)data) + 32768.0f)/65535.0f;
    *((short **)data)=*((short **)data)+1;
    return f;
}
IplImage* DrawHistogram(CvHistogram *hist, float scaleX=5, float scaleY=5)
{
    float histMax = 0;
        cvGetMinMaxHistValue(hist, 0, &histMax, 0, 0);
        IplImage* imgHist = cvCreateImage(cvSize(256*scaleX, 64*scaleY), 8 ,1);
        cvZero(imgHist);

        for(int i=0;i<255;i++)
        {
            float histValue = cvQueryHistValue_1D(hist, i);
            float nextValue = cvQueryHistValue_1D(hist, i+1);

            CvPoint pt1 = cvPoint(i*scaleX, 64*scaleY);
            CvPoint pt2 = cvPoint(i*scaleX+scaleX, 64*scaleY);
            CvPoint pt3 = cvPoint(i*scaleX+scaleX, (64-nextValue*64/histMax)*scaleY);
            CvPoint pt4 = cvPoint(i*scaleX, (64-histValue*64/histMax)*scaleY);

            int numPts = 5;
            CvPoint pts[] = {pt1, pt2, pt3, pt4, pt1};

            cvFillConvexPoly(imgHist, pts, numPts, cvScalar(255));
        }
        return imgHist;
}

int main(int argc, char *argv[])
{
  IplImage* img = 0;
  int height,width,step,channels;
  uchar *data;
  int i,j,k;
  cvNamedWindow( "mainWin", CV_WINDOW_AUTOSIZE );
  cvNamedWindow( "video", CV_WINDOW_AUTOSIZE );

  CvCapture* capture = cvCreateFileCapture( "/media/ENT/Videos/Films/Prometheus 2012 720p (takfilm).mkv" );

  IplImage* frame;
  frame = cvQueryFrame( capture );

  float mult=0.0f;
  char *m=frame->imageData;
  to_float_t func;
  switch (frame->depth) {
  case IPL_DEPTH_32F:
      func=from_32F_F;
      break;
  case IPL_DEPTH_32S:
      func=from_32S_F;
      break;
  case IPL_DEPTH_8S:
      func=from_8S_F;
      break;
  case IPL_DEPTH_8U:
      func=from_8U_F;
      break;
  case IPL_DEPTH_16S:
      func=from_16S_F;
      break;
  case IPL_DEPTH_16U:
      func=from_16U_F;
      break;
  default:
      func=NULL;
  }
  int numBins = 256;
      float range[] = {0, 255};
      float *ranges[] = { range };

      CvHistogram *hist = cvCreateHist(1, &numBins, CV_HIST_ARRAY, ranges, 1);
      IplImage* imgRed = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
      IplImage* imgGreen = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
      IplImage* imgBlue = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);

  while(frame) {
      cvClearHist(hist);
       cvSplit(frame, imgBlue, imgGreen, imgRed, NULL);
       cvCalcHist(&imgRed, hist, 0, 0);
       IplImage* imgHistRed = DrawHistogram(hist);
           //cvClearHist(hist);

      if( !frame ) break;
     cvShowImage( "mainWin", imgHistRed );
     cvShowImage("video", frame);
      char c = cvWaitKey(5);
     // if( c == 27 ) break;
        frame = cvQueryFrame( capture );
        cvReleaseImage(&imgHistRed);
  }
  cvReleaseCapture( &capture );
  cvDestroyWindow("video");
  cvDestroyWindow( "mainWin" );

  return 0;
}
