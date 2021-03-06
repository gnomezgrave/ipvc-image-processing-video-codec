#include <iostream>
#include <cstdio>
#include <opencv2/highgui/highgui.hpp>
#include "ipvc.h"
#include "ipvcplayer.h"

#define TBORDER 10
#define T2BORDER (TBORDER/2)

using namespace cv;
using namespace std;

// Player : Decodes the encoded file
IpvcPlayer::IpvcPlayer(QString inputfile) {
    unsigned frame=0;
    unsigned block_w, block_h;
    namedWindow("Output", CV_WINDOW_AUTOSIZE);


    bool common_header_read=false;
    ushort common_header_size=0;
    vector<uchar> bheader;

    // Read the file header
    ipvc_file_header_t fh;
    FILE *ipvc_file = fopen(inputfile.toStdString().c_str(), "r");
    fread(&fh, 1, sizeof (ipvc_file_header_t), ipvc_file);

    std::cerr << "Header info" << std::endl;
    std::cerr << " height " << fh.height << std::endl;
    std::cerr << " width " << fh.width << std::endl;
    std::cerr << " blocksize " << fh.block_size << std::endl;
    std::cerr << " rate " << (unsigned int) fh.rate << std::endl;

    Mat output(fh.height, fh.width, CV_8UC4);

    block_h = fh.height / fh.block_size;
    block_w = fh.width / fh.block_size;

    uchar type;
    unsigned fid;
    output = Mat::zeros(output.size(),CV_8UC4);

    // Begin the reading
    while (fread(&type, 1, sizeof (uchar), ipvc_file) > 0) {

        fread(&fid, 1, sizeof (unsigned), ipvc_file);

        while (frame<fid) {
            waitKey(1000/fh.rate);
            frame++;
        }
        // Full frames
        if (type == 126) {
            ipvc_frame_full_header_read_t fhx;
            fread(&fhx, 1, sizeof (ipvc_frame_full_header_read_t), ipvc_file);

            uchar jpegbuff[fhx.frame_size];
            fread(&jpegbuff, 1, fhx.frame_size, ipvc_file);
            vector<uchar> vdata(jpegbuff,jpegbuff + fhx.frame_size);

            // JPEG Decode the full frame
            Mat m_buff= imdecode(Mat(vdata),1);

            // Read the full image
            for (int i = 0; i < fh.height; i++) {
                for (int j = 0; j < fh.width; j++) {
                    Vec3b vv= m_buff.at<Vec3b>(i,j);
                    output.at<Vec4b> (i, j)[0] = vv[0];
                    output.at<Vec4b> (i, j)[1] = vv[1];
                    output.at<Vec4b> (i, j)[2] = vv[2];
                    output.at<Vec4b> (i, j)[3] = 255;
                }
            }
        // Frames with new Blocks and/or Block moves
        } else if (type == 122) {

            ipvc_frame_header_read_t ff;

            fread(&ff, 1, sizeof (ipvc_frame_header_read_t), ipvc_file);

            // JPEG header will be stored. Read only one time.
            if (!common_header_read) {
                ipvc_block_header_read_t hb;

                fread(&hb, 1, sizeof(ipvc_block_header_read_t), ipvc_file);
                common_header_size=hb.block_header_size;
                uchar headbuff[common_header_size];

                fread(headbuff,1,(size_t)common_header_size,ipvc_file);
                for (int i=0;i<(int)common_header_size;i++) {
                    bheader.push_back(headbuff[i]);
                }

                common_header_read=true;
            }
            for (ushort bb = 0; bb < ff.blocks; bb++) {

                ipvc_block_read_t bbc;
                fread(&bbc, 1, sizeof (ipvc_block_read_t), ipvc_file);

                unsigned h = (bbc.block_id / block_w) * fh.block_size;
                unsigned w = (bbc.block_id % block_w) * fh.block_size;

                if (h>fh.height ||w>fh.width){
                    break;
                }
                uchar jpegbuff[bbc.block_size]; // JPEG Buffer

                int jpeg_wr=0;
                for (jpeg_wr=0;jpeg_wr<(int)common_header_size;jpeg_wr++) {
                    jpegbuff[jpeg_wr]=bheader[jpeg_wr];
                }
                jpegbuff[jpeg_wr++]=255;
                jpegbuff[jpeg_wr++]=218;
                fread(jpegbuff+jpeg_wr, 1, bbc.block_size - bheader.size() - 2, ipvc_file);

                vector<uchar> vdata(jpegbuff,jpegbuff + bbc.block_size);

                //  JPEG decoder
                Mat m_buff= imdecode(Mat(vdata),1);


                for (int i = 0; i < fh.block_size; i++) {
                    for (int j = 0; j < fh.block_size; j++) {
                        output.at<Vec4b > (h+i, w+j) [0] = m_buff.at<Vec3b>(i,j)[0];
                        output.at<Vec4b > (h+i, w+j) [1] = m_buff.at<Vec3b>(i,j)[1];
                        output.at<Vec4b > (h+i, w+j) [2] = m_buff.at<Vec3b>(i,j)[2];
                        output.at<Vec4b > (h+i, w+j) [3] = 255;

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


                // Block moves in fractional points
                warp_mat = getAffineTransform( srcTri, dstTri );
                warpAffine( block, blockx , warp_mat, Size(fh.block_size+TBORDER,fh.block_size+TBORDER) ,INTER_LINEAR, BORDER_TRANSPARENT);


                // Write the block output to Mat
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
        // Show the output
        imshow("Output", output);

    }

    std::cerr << "Decoding Done" << std::endl;
    destroyWindow("Output");
}
