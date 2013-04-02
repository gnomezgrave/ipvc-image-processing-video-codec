/**
    2013-03-30
    Without using LZMA nearly 50% of space is saved using block based phase correlation.
    (savings depends on video type, if its nearly static better compression can be achieved)

*/

#include <iostream>
#include <cstdio>
#include <cmath>
#include <stdint.h>
//#include <lzma.h>
#include "ipvc.h"
#include <opencv2/highgui/highgui.hpp>


#define OUT_FILE argv[2]

#define PIXEL(jump, bigj, smallj) (((jump)*(bigj) + (smallj))*3)
#define ARR2D(jump, bigj, smallj) (((jump)*(bigj) + (smallj)))

#define AVG(a,b,c) (((a)+(b)+(c))/3.0)

enum ipvc_modified_by_t{IPVC_DIFF, IPVC_PC, IPVC_STATIC};
/* COMPRESSION SETTINGS */

/* analogous to xz CLI options: -0 to -9 */
#define COMPRESSION_LEVEL 0

/* boolean setting, analogous to xz CLI option: -e */
#define COMPRESSION_EXTREME false

/* see: /usr/include/lzma/check.h LZMA_CHECK_* */
#define INTEGRITY_CHECK LZMA_CHECK_NONE

/* read/write buffer sizes */
#define IN_BUF_MAX	4096
#define OUT_BUF_MAX	4096

using namespace cv;
using namespace std;

int mask[3] = {-1,0,1};

int blurmask3[3][3] = { { 1, 2, 1 } ,
                        { 2, 4, 2 } ,
                        { 1, 2, 1 }
                      };

int main(int argc, char *argv[])
{
    /*uint32_t preset = COMPRESSION_LEVEL | (COMPRESSION_EXTREME ? LZMA_PRESET_EXTREME : 0);
    lzma_check check = INTEGRITY_CHECK;
    lzma_stream strm = LZMA_STREAM_INIT;
    uint8_t *in_buf;
    uint8_t *out_buf;
    size_t in_len;
    size_t out_len;
    bool in_finished = false;
    bool out_finished = false;
    lzma_action action;
    lzma_ret ret_xz;*/
    char press=-1;
    float quality=0.1f; // number between 0 and 1, can't be 0

    unsigned changes;
    unsigned current_frame=0;

    FILE *ipvc_file = fopen(OUT_FILE, "w");

    namedWindow( "video_ori", CV_WINDOW_AUTOSIZE );
    namedWindow( "video_out", CV_WINDOW_AUTOSIZE );
    namedWindow( "video_grey", CV_WINDOW_AUTOSIZE );

    VideoCapture capture;
    capture.open( argv[1] );
    Mat frame;

    capture >> frame;
    if (frame.type()!= CV_8UC3 ) {
        cout<<"Image type not supported."<<endl;
        return 0;
    }

    unsigned height = frame.rows;
    unsigned width = frame.cols;
    unsigned total = height*width;
    unsigned total_size = total*3;
    float ftotal = total*1.0f;

    unsigned blocks_h = height/BLOCK_SIZE;
    unsigned blocks_w = width/BLOCK_SIZE;

    bool mark[blocks_h][blocks_w];
    ipvc_modified_by_t modd[blocks_h][blocks_w];

    Mat m_image1(height, width, CV_8UC3);
    Mat m_image2(height, width, CV_8UC3);
    Mat m_output(height, width, CV_8UC3);

    Mat m_complexity_b(BLOCK_SIZE,BLOCK_SIZE,CV_8UC1);
    Mat m_grey_b(BLOCK_SIZE,BLOCK_SIZE,CV_8UC1);
    Mat m_output_greys_f1[blocks_h][blocks_w];
    Mat m_output_greys_f2[blocks_h][blocks_w];

    Mat m_tmp(height,width,CV_8UC4);

    Mat m_output_greys[blocks_h][blocks_w];
    Mat m_temp[blocks_h][blocks_w];

    Mat m_translate[blocks_h][blocks_w];

    for (unsigned i=0;i<blocks_h;i++) {
        for (unsigned j=0;j<blocks_w;j++) {

            m_translate[i][j].create(2,3,CV_32FC1);

            m_output_greys[i][j].create(BLOCK_SIZE,BLOCK_SIZE,CV_8UC1);
            m_output_greys_f1[i][j].create(BLOCK_SIZE,BLOCK_SIZE, CV_32FC1);
            m_output_greys_f2[i][j].create(BLOCK_SIZE,BLOCK_SIZE, CV_32FC1);

            m_temp[i][j].create(BLOCK_SIZE,BLOCK_SIZE,CV_8UC3);

            m_translate[i][j].at<float>(0,0) = 1.0f;
            m_translate[i][j].at<float>(0,1) = 0.0f;
            m_translate[i][j].at<float>(1,0) = 0.0f;
            m_translate[i][j].at<float>(1,1) = 1.0f;

            m_translate[i][j].at<float>(0,2)=0.0f;
            m_translate[i][j].at<float>(1,2)=0.0f;
        }
    }



    Mat *ffts,*prevffts,*greys_f,*prevgreys_f;

    Mat *image=NULL,*previmage=NULL, *blurred,*changed,*prevchanged;
    Mat *output;

    image=&m_image1;
    previmage=&m_image2;
    output=&m_output;

    greys_f=&m_output_greys_f1[0][0];
    prevgreys_f=&m_output_greys_f2[0][0];

    //in_buf = (uint8_t *) m_output.data;
   // out_buf = (uint8_t *) malloc(height*width*sizeof(uchar)*3);

    for (unsigned i=0;i<height;i++){
        for (unsigned j=0;j<width;j++) {
            image->at<Vec3b>(i,j) = Vec3b(0,0,0);
            previmage->at<Vec3b>(i,j) = Vec3b(0,0,0);

            output->at<Vec3b>(i,j)=Vec3b(0,0,0);

            m_output_greys_f1[i%blocks_h][j%blocks_w].at<float>(i%BLOCK_SIZE,j%BLOCK_SIZE)=0.0f;
            m_output_greys_f2[i%blocks_h][j%blocks_w].at<float>(i%BLOCK_SIZE,j%BLOCK_SIZE)=0.0f;

        }
    }

    /* initialize xz encoder */
    /*
    ret_xz = lzma_easy_encoder (&strm, preset, check);
    if (ret_xz != LZMA_OK) {
        std::cerr <<"lzma_easy_encoder error: "<< ret_xz<<std::endl;
        return -1;
    }
    */
    ipvc_file_header_t fh;
    ipvc_frame_header_t frh;
    fh.height = height;
    fh.width = width;

    fwrite(&fh, 1,sizeof(ipvc_file_header_t),ipvc_file);
    std::cout<<"sd"<<endl;

    for (unsigned i=0;i<blocks_h;i++){
        for (unsigned j=0;j<blocks_w;j++) {
            for (int a=0;a<BLOCK_SIZE;a++) {
                for (int b=0;b<BLOCK_SIZE;b++) {
                    m_temp[i][j].at<Vec3b>(a,b) = frame.at<Vec3b>(BLOCK_SIZE*i+a,BLOCK_SIZE*j+b);
                }
            }

        }
    }

    capture >> frame;
    float avg=0.0f;
    frame.copyTo(m_output);
    current_frame=2;
    while(!frame.empty() && press==-1) {

        unsigned frame_differences=0;
       // m_output = Mat::zeros(height, width, CV_8UC3);
        for (unsigned i=0;i<height;i++) {
            for (unsigned j=0;j<width;j++) {

                Vec3b is = frame.at<Vec3b>(i,j);
                image->at<Vec3b>(i,j) = is;
                Vec3b ps = previmage->at<Vec3b>(i,j);
                if (abs(is[0]-ps[0]) > 5 &&
                        abs(is[1]-ps[1]) > 5 &&
                        abs(is[2]-ps[2]) > 5)
                {
                    frame_differences++;
                }
            }
        }
        float p_diff = frame_differences*1.0f/(width*height);
        if (p_diff>0.8f) {
            // full frame
            ipvc_frame_full_header_t frh;
            fwrite(&frh, 1,sizeof(ipvc_frame_full_header_t),ipvc_file);
            char data[width*height*3];
            fwrite(&data, 1,width*height*3,ipvc_file);
            image->copyTo(m_output);
            image->copyTo(*previmage);
            cout<<"full"<<endl;
        }
        for (unsigned i=0;i<blocks_h;i++){
            for (unsigned j=0;j<blocks_w;j++) {
                bool changed=false;

                for (int a=0;a<BLOCK_SIZE;a++) {
                    for (int b=0;b<BLOCK_SIZE;b++) {
                        Vec3b p= frame.at<Vec3b>(i*BLOCK_SIZE+a,j*BLOCK_SIZE+b);
                        image->at<Vec3b>(i*BLOCK_SIZE+a,j*BLOCK_SIZE+b) = p;
                        m_temp[i][j].at<Vec3b>(a,b)=previmage->at<Vec3b>(i*BLOCK_SIZE+a,j*BLOCK_SIZE+b);
                        float tmp = (p[0]+p[1]+p[2])/3.0;
                        greys_f[ARR2D(blocks_w,i,j)].at<float>(a,b) = tmp;
                        m_grey_b.at<uchar>(a,b) = (uchar) tmp;
                    }
                }

                Point2d pc(0,0);
                double d;
                pc=phaseCorrelate(prevgreys_f[ARR2D(blocks_w,i,j)],greys_f[ARR2D(blocks_w,i,j)]);

                unsigned correct=0;
                unsigned complexity=0;
                bool edge=false;
                mark[i][j] = false;
                Canny(m_grey_b,m_complexity_b,100,250);
                if (fabs(pc.x)+ fabs(pc.y)<5.0f){
                    for (int a=0;a<BLOCK_SIZE;a++){
                        for (int b=0;b<BLOCK_SIZE;b++){
                            int h=BLOCK_SIZE*i+a+(int)pc.x;
                            int w=BLOCK_SIZE*j+b+(int)pc.y;
                            if (h<0||h>=height||w<0||w>=width)
                                edge=true;
                            else{
                                if (abs(previmage->at<Vec3b>(h,w)[0]-image->at<Vec3b>(h,w)[0]) < 4 &&
                                    abs(previmage->at<Vec3b>(h,w)[1]-image->at<Vec3b>(h,w)[1]) < 4 &&
                                    abs(previmage->at<Vec3b>(h,w)[2]-image->at<Vec3b>(h,w)[2]) < 4){
                                    correct++;
                                }
                            }
                            if (m_complexity_b.at<uchar>(a,b)>0){
                                complexity++;
                            }
                        }
                    }

                    if (complexity > BLOCK_SIZE*BLOCK_SIZE*0.01 && correct>BLOCK_SIZE*BLOCK_SIZE*0.8 && !edge){

                         for (int a=0;a<BLOCK_SIZE;a++){
                            for (int b=0;b<BLOCK_SIZE;b++){
                                int h=BLOCK_SIZE*i+a;
                                int w=BLOCK_SIZE*j+b;
                                m_temp[i][j].at<Vec3b>(a,b) = output->at<Vec3b>(h,w);

                            }
                        }

                        for (int a=0;a<BLOCK_SIZE;a++){
                            for (int b=0;b<BLOCK_SIZE;b++){

                                int h=BLOCK_SIZE*i+a;
                                int w=BLOCK_SIZE*j+b;
                                int hh=h+((int)pc.y);
                                int ww=w+((int)pc.x);
                                if (hh>=0 && ww>=0 && hh<height && ww<width)
                                    output->at<Vec3b>(hh,ww) = m_temp[i][j].at<Vec3b>(a,b);//.mul(Vec3b(1,0,1));


                            }
                        }

                        mark[i][j]=true;
                        changed=true;
                        modd[i][j]=IPVC_PC;
                        ipvc_block_move_t block;
                        fwrite(&block, 1,sizeof(ipvc_block_move_t),ipvc_file);

                    } else if  (correct > BLOCK_SIZE*BLOCK_SIZE*0.8 && !edge) {
                        mark[i][j]=true;
                        modd[i][j]=IPVC_STATIC;
                    }

                }

                unsigned differences=0;
                //cout<<mark[i][j]<<endl;
                if (!mark[i][j]){
                    for (int a=0;a<BLOCK_SIZE;a++) {
                        for (int b=0;b<BLOCK_SIZE;b++) {
                            Vec3b p= image->at<Vec3b>(i*BLOCK_SIZE+a,j*BLOCK_SIZE+b);
                            Vec3b d= previmage->at<Vec3b>(i*BLOCK_SIZE+a,j*BLOCK_SIZE+b);
                            if ((abs(p[0]-d[0]) > 5) &&
                                (abs(p[1]-d[1]) > 5) &&
                                (abs(p[2]-d[2]) > 5) ) {
                                differences++;

                            }

                        }
                    }
                    if (differences > 0) {
                        for (int a=0;a<BLOCK_SIZE;a++) {
                            for (int b=0;b<BLOCK_SIZE;b++) {
                                output->at<Vec3b>(i*BLOCK_SIZE+a,j*BLOCK_SIZE+b) = image->at<Vec3b>(i*BLOCK_SIZE+a,j*BLOCK_SIZE+b);//.mul(Vec3b(0,1,1));
                            }
                        }
                        changed=true;
                        modd[i][j]=IPVC_DIFF;
                        ipvc_block_t block;
                        fwrite(&block, 1,sizeof(ipvc_block_t),ipvc_file);
                    }

                }

                if (changed && 0) {
                    if (modd[i][j]==IPVC_DIFF){ //purple
                        for (int a=0;a<BLOCK_SIZE;a++) {
                            for (int b=0;b<BLOCK_SIZE;b++) {
                                output->at<Vec3b>(i*BLOCK_SIZE+a,j*BLOCK_SIZE+b) = output->at<Vec3b>(i*BLOCK_SIZE+a,j*BLOCK_SIZE+b).mul(Vec3b(1,0,1));
                            }
                        }
                    } else if (modd[i][j]==IPVC_PC){ //blue
                        for (int a=0;a<BLOCK_SIZE;a++) {
                            for (int b=0;b<BLOCK_SIZE;b++) {
                                output->at<Vec3b>(i*BLOCK_SIZE+a,j*BLOCK_SIZE+b) = output->at<Vec3b>(i*BLOCK_SIZE+a,j*BLOCK_SIZE+b).mul(Vec3b(1,0,0));
                            }
                        }

                    } else {
                        for (int a=0;a<BLOCK_SIZE;a++) { //yellow
                            for (int b=0;b<BLOCK_SIZE;b++) {
                                output->at<Vec3b>(i*BLOCK_SIZE+a,j*BLOCK_SIZE+b) = output->at<Vec3b>(i*BLOCK_SIZE+a,j*BLOCK_SIZE+b).mul(Vec3b(0,1,1));
                            }
                        }
                    }

                }

            }
        }



        imshow("video_ori", frame);
        imshow("video_out", m_output);

        press= waitKey(1);

        capture>>frame;
        current_frame++;
        if (image==&m_image1){
            image=&m_image2;
            previmage=&m_image1;
            greys_f=&m_output_greys_f2[0][0];
            prevgreys_f=&m_output_greys_f1[0][0];

        } else {
            image=&m_image1;
            previmage=&m_image2;
            greys_f=&m_output_greys_f1[0][0];
            prevgreys_f=&m_output_greys_f2[0][0];
        }
    }
    cout<<total_size<<endl;
    fclose(ipvc_file);
    //lzma_end (&strm);

    return 0;
}


