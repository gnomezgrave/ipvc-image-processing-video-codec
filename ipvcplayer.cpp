#include <iostream>
#include <cstdio>
#include <opencv2/highgui/highgui.hpp>
#include "ipvc.h"
#include "ipvcplayer.h"
#include "ui_ipvcplayer.h"

using namespace cv;
using namespace std;

IpvcPlayer::IpvcPlayer(char *inputfile) {

    namedWindow("Output", CV_WINDOW_AUTOSIZE );
    ipvc_file_header_t fh;
    FILE *ipvc_file = fopen(inputfile, "r");
    fread(&fh,1,sizeof(ipvc_file_header_t),ipvc_file);
    std::cout<<"Header info"<<std::endl;
    std::cout<<" height "<<fh.height<<std::endl;
    std::cout<<" width "<<fh.width<<std::endl;
    std::cout<<" blocksize "<<fh.block_size<<std::endl;
    std::cout<<" rate "<<(unsigned int)fh.rate<<std::endl;

    Mat output(fh.height,fh.width,CV_8UC3);

    uchar type;
    while(!feof(ipvc_file)){
        fread(&type,1,sizeof(uchar),ipvc_file);
        if((uchar)type==126){
            unsigned id;
            fread(&id,1,sizeof(int),ipvc_file);
            for(int i=0;i<fh.height;i++){
                for(int j=0;j<fh.width;j++){
                    uchar r,g,b;
                    fread(&r,1,1,ipvc_file);
                    fread(&g,1,1,ipvc_file);
                    fread(&b,1,1,ipvc_file);
                    output.at<Vec3b>(i,j)=Vec3b(r,g,b);
                }
            }
            imshow("Output", output);
            waitKey(1000);
            std::cout<<"Full frame"<<std::endl;

        }else if((uchar)type==122){
            ipvc_frame_header_read_t ff;
            fread(&ff,1,sizeof(ipvc_frame_header_read_t),ipvc_file);
            std::cout<<"Move"<<std::endl;
        }
    }
    std::cout<<"Decoding Done"<<std::endl;
}

