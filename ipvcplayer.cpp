#include <iostream>
#include <cstdio>
#include <opencv2/highgui/highgui.hpp>
#include "ipvc.h"
#include "ipvcplayer.h"

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

    Mat output(fh.height, fh.width, CV_8UC3);
    block_h = fh.height / fh.block_size;
    block_w = fh.width / fh.block_size;

    uchar type;
    while (!feof(ipvc_file)) {
        fread(&type, 1, sizeof (uchar), ipvc_file);

        if (type == 126) {
            std::cout << "Full frame" << std::endl;

            unsigned id;
            fread(&id, 1, sizeof (unsigned), ipvc_file);

            std::cout << "frame " << id << std::endl;
            printf("%x", id);
            for (int i = 0; i < fh.height; i++) {
                for (int j = 0; j < fh.width; j++) {
                    uchar rgb[3];
                    fread(&rgb, 1, 3, ipvc_file);
                    output.at<Vec3b > (i, j) = Vec3b(rgb[0], rgb[1], rgb[2]);
                }
            }
            imshow("Output", output);

            waitKey(1000 / fh.rate);

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

                for (int i = 0; i < fh.block_size; i++) {
                    for (int j = 0; j < fh.block_size; j++) {
                        uchar rgb[3];
                        fread(&rgb, 1, 3, ipvc_file);
                        unsigned h = (block_id / block_w) * fh.block_size + i;
                        unsigned w = (block_id % block_w) * fh.block_size + j;

                        output.at<Vec3b > (h, w) = Vec3b(rgb[0], rgb[1], rgb[2]);
                    }
                }

            }
            imshow("Output", output);
            waitKey(1000 / fh.rate);
            for (ushort bb = 0; bb < ff.block_moves; bb++) {
                ipvc_block_move_t mv;
                fread(&mv, 1, sizeof (ipvc_block_move_t), ipvc_file);
                for (int i = 0; i < fh.block_size; i++) {
                    for (int j = 0; j < fh.block_size; j++) {
                        unsigned h = (mv.block_id / block_w) * fh.block_size + i;
                        unsigned w = (mv.block_id % block_w) * fh.block_size + j;
                        output.at<Vec3b > (h + (int) mv.move_h, w + (int) mv.move_w) = output.at<Vec3b > (h, w);
                    }
                }
            }

        }
    }
    std::cout << "Decoding Done" << std::endl;
}
