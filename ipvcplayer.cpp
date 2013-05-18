#include <iostream>
#include <cstdio>
#include <opencv2/highgui/highgui.hpp>
#include "ipvc.h"
#include "ipvcplayer.h"

#define TBORDER 10
#define T2BORDER (TBORDER/2)
using namespace cv;
using namespace std;

IpvcPlayer::IpvcPlayer(char *inputfile) {
    unsigned block_w, block_h;
    namedWindow("Output", CV_WINDOW_AUTOSIZE);
    ipvc_file_header_t fh;
    FILE *ipvc_file = fopen(inputfile, "r");
    fread(&fh, 1, sizeof (ipvc_file_header_t), ipvc_file);
    std::cout << "Header info" << std::endl;
    std::cout << " height " << fh.height << std::endl;
    std::cout << " width " << fh.width << std::endl;
    std::cout << " blocksize " << fh.block_size << std::endl;
    std::cout << " rate " << (unsigned int) fh.rate << std::endl;

    Mat output(fh.height, fh.width, CV_8UC4);

    block_h = fh.height / fh.block_size;
    block_w = fh.width / fh.block_size;

    uchar type;
    while (!feof(ipvc_file)) {
        fread(&type, 1, sizeof (uchar), ipvc_file);

        if (type == 126) {
            unsigned id;
            fread(&id, 1, sizeof (unsigned), ipvc_file);
            std::cout << "Full frame" << std::endl;
            printf("%x", id);
            for (int i = 0; i < fh.height; i++) {
                for (int j = 0; j < fh.width; j++) {
                    uchar rgb[3];
                    fread(&rgb, 1, 3, ipvc_file);
                    output.at<Vec4b > (i, j) = Vec4b(rgb[0], rgb[1], rgb[2],255);
                }
            }


        } else if (type == 122) {

            std::cout << "Move" << std::endl;
            ipvc_frame_header_read_t ff;

            fread(&ff, 1, sizeof (ipvc_frame_header_read_t), ipvc_file);
            std::cout << "frame " << ff.frame_id << std::endl;
            std::cout << "blocks " << ff.blocks << std::endl;
            std::cout << "moves " << ff.block_moves << std::endl;

            for (ushort bb = 0; bb < ff.blocks; bb++) {
                short block_id;
                fread(&block_id, 1, sizeof (short), ipvc_file);
                unsigned h = (block_id / block_w) * fh.block_size;
                unsigned w = (block_id % block_w) * fh.block_size;
                std::cout<<h<<" "<<w<<std::endl;
                for (int i = 0; i < fh.block_size; i++) {
                    for (int j = 0; j < fh.block_size; j++) {
                        uchar rgb[3];
                        fread(&rgb, 1, 3, ipvc_file);


                        output.at<Vec4b > (h+i, w+j) = Vec4b(rgb[0], rgb[1], rgb[2],255);
                    }
                }

            }

            Point2f srcTri[4];
            Point2f dstTri[4];
            Mat warp_mat( 2, 3, CV_32FC1 );

            for (ushort bb = 0; bb < ff.block_moves; bb++) {

                ipvc_block_move_t mv;
                fread(&mv, 1, sizeof (ipvc_block_move_t), ipvc_file);

                Mat block(fh.block_size+TBORDER, fh.block_size+TBORDER, CV_8UC4);
                Mat blockx(fh.block_size+TBORDER, fh.block_size+TBORDER, CV_8UC4);
                unsigned hh = (mv.block_id / block_w);
                unsigned ww = (mv.block_id % block_w);

                block = Mat::zeros(fh.block_size + TBORDER,fh.block_size + TBORDER,CV_8UC4);

                for (int i=0;i<fh.block_size;i++) {
                    for (int j=0;j<fh.block_size;j++) {
                        block.at<Vec4b>(i+T2BORDER,j+T2BORDER)=output.at<Vec4b>(hh+i,ww+j);
                    }
                }


                srcTri[0] = Point2f( T2BORDER , T2BORDER);
                srcTri[1] = Point2f( T2BORDER, T2BORDER+ fh.block_size);
                srcTri[2] = Point2f( T2BORDER+fh.block_size,T2BORDER+ fh.block_size);
                srcTri[3] = Point2f( T2BORDER+fh.block_size, T2BORDER);

                dstTri[0] = Point2f( mv.move_h, (float)mv.move_w );
                dstTri[1] = Point2f( mv.move_h, (float)mv.move_w+fh.block_size );
                dstTri[2] = Point2f( mv.move_h+(float)fh.block_size, (float)mv.move_w+fh.block_size );
                dstTri[3] = Point2f( mv.move_h+(float)fh.block_size, (float)mv.move_w);


                warp_mat = getAffineTransform( srcTri, dstTri );
                warpAffine( block, blockx , warp_mat, Size(fh.block_size+TBORDER,fh.block_size+TBORDER) ,INTER_LINEAR, BORDER_TRANSPARENT);

                for (int i=0;i<fh.block_size;i++){
                    for (int j=0;j<fh.block_size;j++){

                        Vec4b xx=block.at<Vec4b>(i+T2BORDER,j+T2BORDER);
                        if (xx[3]>0){
                             output.at<Vec4b>(hh+i,ww+j)=xx;
                        }
                    }
                }


            }
        }
        waitKey(1);
        imshow("Output", output);
    }

    std::cout << "Decoding Done" << std::endl;
}
