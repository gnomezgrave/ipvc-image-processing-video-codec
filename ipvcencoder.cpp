#include "ipvcencoder.h"

#include <iostream>
#include <list>
#include <cstdio>
#include <cmath>
#include <stdint.h>
#include <opencv2/highgui/highgui.hpp>

#include "ipvcmain.h"
#include "ipvc.h"

#define PIXEL(jump, bigj, smallj) (((jump)*(bigj) + (smallj))*3)
#define ARR2D(jump, bigj, smallj) (((jump)*(bigj) + (smallj)))

#define AVG(a,b,c) (((a)+(b)+(c))/3.0)
enum ipvc_modified_by_t {
    IPVC_DIFF, IPVC_PC
};

using namespace cv;
using namespace std;

int mask[3] = {-1, 0, 1};

// Encoder will encode the input video file.
// A seperate Encoder object will be created for each video file.
IpvcEncoder::IpvcEncoder(IpvcMain* parent,QString inputfile, QString outputfile) {

    // lists that keep block moves and new block writes until they are
    // written to the file
    list<ipvc_block_move_t> l_block_moves;
    list<ipvc_block_t> l_blocks;

    l_block_moves.clear();
    l_blocks.clear();

    frameSize=frameSizeWithoutJpeg=0;

    bool common_header_written=false;
    ushort common_header_size=0;
    vector<uchar> bheader;
    unsigned current_frame = 0;

    //Output file
    FILE *ipvc_file = fopen(outputfile.toStdString().c_str(), "w");


    //Input video capture
    VideoCapture capture;
    capture.open(inputfile.toStdString());
    Mat frame;

    capture >> frame;
    if (frame.empty()) {
        cerr << "Can\'t extract frame data"<<endl;
        return;
    }
    if (frame.type() != CV_8UC3) {
        cerr << "Image type not supported. Is of type:" << frame.type() << endl;
        return;
    }

    if(!parent->getChartState()){
       namedWindow("Original_Video", CV_WINDOW_AUTOSIZE);
       namedWindow("Overlay_Video", CV_WINDOW_AUTOSIZE);
       namedWindow("Output_Video", CV_WINDOW_AUTOSIZE);
    }


    unsigned height = frame.rows;
    unsigned width = frame.cols;
    unsigned total = height*width;
    unsigned total_size = total * 3;
    float ftotal = total_size * 1.0f;

    unsigned blocks_h = height / BLOCK_SIZE;
    unsigned blocks_w = width / BLOCK_SIZE;


    cerr << "Frame" << endl;
    cerr << " Height:" << height << endl;
    cerr << " Width:" << width << endl;
    cerr << " Block" << endl;
    cerr << "  Rows:" << blocks_h << endl;
    cerr << "  Cols:" << blocks_w << endl;

    bool mark[blocks_h][blocks_w];
    ipvc_modified_by_t modd[blocks_h][blocks_w];
    // image1 and image2 provide buffers to keep the latest two
    // frames read from the input in memory without copying
    // image and previmage pointers are swapped on each read of a
    // new frame
    Mat m_image1(height, width, CV_8UC3);
    Mat m_image2(height, width, CV_8UC3);

    // output image buffers
    Mat m_output(height, width, CV_8UC3);
    Mat m_output_overlay(height, width, CV_8UC3);
    Mat m_final(height, width, CV_8UC3);

    // stores greyscale per block and edges per block(complexity)
    Mat m_complexity_b(BLOCK_SIZE, BLOCK_SIZE, CV_8UC1);
    Mat m_grey_b(BLOCK_SIZE, BLOCK_SIZE, CV_8UC1);

    // greyscale block buffers again using the buffer swaping used
    // in image1 and image2
    Mat m_output_greys_f1[blocks_h][blocks_w];
    Mat m_output_greys_f2[blocks_h][blocks_w];

    // temp buffer used at various places for various operations
    Mat m_temp[blocks_h][blocks_w];

    // translation matrices for block moves
    Mat m_translate[blocks_h][blocks_w];

    // single block buffer
    Mat m_block(BLOCK_SIZE,BLOCK_SIZE,CV_8UC3);


    //initialize buffers
    for (unsigned i = 0; i < blocks_h; i++) {
        for (unsigned j = 0; j < blocks_w; j++) {

            m_translate[i][j].create(2, 3, CV_32FC1);

            m_output_greys_f1[i][j].create(BLOCK_SIZE, BLOCK_SIZE, CV_32FC1);
            m_output_greys_f2[i][j].create(BLOCK_SIZE, BLOCK_SIZE, CV_32FC1);

            m_temp[i][j].create(BLOCK_SIZE, BLOCK_SIZE, CV_8UC3);

            m_translate[i][j].at<float>(0, 0) = 1.0f;
            m_translate[i][j].at<float>(0, 1) = 0.0f;
            m_translate[i][j].at<float>(1, 0) = 0.0f;
            m_translate[i][j].at<float>(1, 1) = 1.0f;

            m_translate[i][j].at<float>(0, 2) = 0.0f;
            m_translate[i][j].at<float>(1, 2) = 0.0f;
        }
    }

    Mat *greys_f, *prevgreys_f;

    Mat *image = NULL, *previmage = NULL;
    Mat *output;

    image = &m_image1;
    previmage = &m_image2;
    output = &m_output;

    greys_f = &m_output_greys_f1[0][0];
    prevgreys_f = &m_output_greys_f2[0][0];

    for (unsigned i = 0; i < height; i++) {
        for (unsigned j = 0; j < width; j++) {
            image->at<Vec3b > (i, j) = frame.at<Vec3b>(i,j);
            previmage->at<Vec3b > (i, j) = Vec3b(0, 0, 0);
            output->at<Vec3b > (i, j) = Vec3b(0, 0, 0);

            m_output_greys_f1[i % blocks_h][j % blocks_w].at<float>(i % BLOCK_SIZE, j % BLOCK_SIZE) = 0.0f;
            m_output_greys_f2[i % blocks_h][j % blocks_w].at<float>(i % BLOCK_SIZE, j % BLOCK_SIZE) = 0.0f;
        }
    }

    ipvc_file_header_t fh;
    fh.height = height;
    fh.width = width;
    fh.block_size = BLOCK_SIZE;
    fh.rate = 25;
    fwrite(&fh, 1, sizeof (ipvc_file_header_t), ipvc_file);

    for (unsigned i = 0; i < blocks_h; i++) {
        for (unsigned j = 0; j < blocks_w; j++) {
            for (int a = 0; a < BLOCK_SIZE; a++) {
                for (int b = 0; b < BLOCK_SIZE; b++) {
                    m_temp[i][j].at<Vec3b > (a, b) = frame.at<Vec3b > (BLOCK_SIZE * i + a, BLOCK_SIZE * j + b);
                }
            }
        }
    }


    frame.copyTo(m_output);
    current_frame = 2;
    while (!frame.empty()) {
        unsigned frame_differences = 0;
        m_output_overlay = Mat::zeros(height, width, CV_8UC3);
        for (unsigned i = 0; i < height; i++) {
            for (unsigned j = 0; j < width; j++) {

                Vec3b is = frame.at<Vec3b > (i, j);
                image->at<Vec3b > (i, j) = is;
                Vec3b ps = previmage->at<Vec3b > (i, j);
                if ((abs(is[0] - ps[0]) > 5) &&
                        (abs(is[1] - ps[1]) > 5) &&
                        (abs(is[2] - ps[2]) > 5)) {
                    frame_differences++;
                }
            }
        }
        float p_diff = frame_differences * 1.0f / (width * height);
        if (p_diff > 0.8f) {
            // full frame
            ipvc_frame_full_header_t frh;

            vector<uchar> ff;
            imencode(".jpg",*image,ff);
            frh.frame_type = 126;
            frh.frame_id = current_frame;
            frh.frame_size = (unsigned)ff.size();
            fwrite(&frh, 1, sizeof (ipvc_frame_full_header_t), ipvc_file);
            frameSize+=sizeof(frh);

            image->copyTo(m_output);
            image->copyTo(*previmage);

            fwrite((uchar *)ff.data(),1,ff.size(),ipvc_file);
            frameSize+=ff.size();
            frameSizeWithoutJpeg+=sizeof(frh)+total_size;
        }

        for (unsigned i = 0; i < blocks_h; i++) {
            for (unsigned j = 0; j < blocks_w; j++) {
                bool changed = false;
                unsigned differences_frame = 0;

                for (int a = 0; a < BLOCK_SIZE; a++) {
                    for (int b = 0; b < BLOCK_SIZE; b++) {
                        Vec3b p = frame.at<Vec3b > (i * BLOCK_SIZE + a, j * BLOCK_SIZE + b);
                        image->at<Vec3b > (i * BLOCK_SIZE + a, j * BLOCK_SIZE + b) = p;
                        m_temp[i][j].at<Vec3b > (a, b) = previmage->at<Vec3b > (i * BLOCK_SIZE + a, j * BLOCK_SIZE + b);
                        float tmp = (p[0] + p[1] + p[2]) / 3.0;
                        greys_f[ARR2D(blocks_w, i, j)].at<float>(a, b) = tmp;
                        m_grey_b.at<uchar > (a, b) = (uchar) tmp;


                        Vec3b oo = m_output.at<Vec3b > (i * BLOCK_SIZE + a, j * BLOCK_SIZE + b);

                        if (abs(p[0] - oo[0]) > 1 && abs(p[1] - oo[1]) > 1 && abs(p[2] - oo[2]) > 1)
                            differences_frame++;
                    }
                }

                Point2d pc(0, 0);
                pc=phaseCorrelateX(prevgreys_f[ARR2D(blocks_w,i,j)],greys_f[ARR2D(blocks_w,i,j)]);

                unsigned correct = 0;
                unsigned complexity = 0;
                bool edge = false;
                mark[i][j] = false;
                Canny(m_grey_b, m_complexity_b, 100, 250);
                if (fabs(pc.x) + fabs(pc.y) < 10.0f) {
                    for (int a = 0; a < BLOCK_SIZE; a++) {
                        for (int b = 0; b < BLOCK_SIZE; b++) {
                            int h = BLOCK_SIZE * i + a + (int) pc.x;
                            int w = BLOCK_SIZE * j + b + (int) pc.y;
                            if (h < 0 || h >= height || w < 0 || w >= width){
                                edge = true;
                            } else {
                                Vec3b pp = image->at<Vec3b > (h, w);
                                Vec3b ii = output->at<Vec3b > (h, w);
                                if (abs(pp[0] - ii[0]) < differences_frame * 38 / (BLOCK_SIZE * BLOCK_SIZE) &&
                                        abs(pp[1] - ii[1]) < differences_frame * 38 / (BLOCK_SIZE * BLOCK_SIZE) &&
                                        abs(pp[2] - ii[2]) < differences_frame * 38 / (BLOCK_SIZE * BLOCK_SIZE)) {
                                    correct++;
                                }
                            }
                            if (m_complexity_b.at<uchar > (a, b) > 0) {
                                complexity++;
                            }
                        }
                    }
                    // for high correction in a block move to a block move
                    if (complexity > BLOCK_SIZE * BLOCK_SIZE * 0.001 && correct > BLOCK_SIZE * BLOCK_SIZE * 0.95 && !edge) {

                        for (int a = 0; a < BLOCK_SIZE; a++) {
                            for (int b = 0; b < BLOCK_SIZE; b++) {
                                int h = BLOCK_SIZE * i + a;
                                int w = BLOCK_SIZE * j + b;
                                m_temp[i][j].at<Vec3b > (a, b) = output->at<Vec3b > (h, w);

                            }
                        }

                        for (int a = 0; a < BLOCK_SIZE; a++) {
                            for (int b = 0; b < BLOCK_SIZE; b++) {

                                int h = BLOCK_SIZE * i + a;
                                int w = BLOCK_SIZE * j + b;
                                int hh = h + ((int) pc.y);
                                int ww = w + ((int) pc.x);
                                if (hh >= 0 && ww >= 0 && hh < height && ww < width)
                                    output->at<Vec3b > (hh, ww) = m_temp[i][j].at<Vec3b > (a, b); //mul(Vec3b(1,0,1));

                            }
                        }

                        mark[i][j] = true;
                        changed = true;
                        modd[i][j] = IPVC_PC;
                        ipvc_block_move_t block;
                        block.block_id = ARR2D(blocks_w, i, j);
                        block.move_h = (float) pc.y;
                        block.move_w = (float) pc.x;

                        l_block_moves.push_back(block);

                    } else if (correct > BLOCK_SIZE * BLOCK_SIZE * 0.9) {
                        mark[i][j] = false;
                        changed = false;
                    }

                }

                unsigned differences = 0;

                if (!mark[i][j]) {
                    for (int a = 0; a < BLOCK_SIZE; a++) {
                        for (int b = 0; b < BLOCK_SIZE; b++) {
                            Vec3b p = m_output.at<Vec3b > (i * BLOCK_SIZE + a, j * BLOCK_SIZE + b);
                            Vec3b d = image->at<Vec3b > (i * BLOCK_SIZE + a, j * BLOCK_SIZE + b);
                            if ((abs(p[0] - d[0]) > 5) &&
                                    (abs(p[1] - d[1]) > 5) &&
                                    (abs(p[2] - d[2]) > 5)) {
                                differences++;

                            }

                        }
                    }
                    // check for differences in non marked(not block moves) in order to write
                    // new blocks
                    if (differences >0) {
                        ipvc_block_t block;
                        block.block_id = ARR2D(blocks_w, i, j);
                        for (int a = 0; a < BLOCK_SIZE; a++) {
                            for (int b = 0; b < BLOCK_SIZE; b++) {
                                int h = i*BLOCK_SIZE;
                                int w = j*BLOCK_SIZE;
                                Vec3b nv = image->at<Vec3b > (h + a, w + b);
                                output->at<Vec3b > (h + a, w + b) = nv; //.mul(Vec3b(0,1,1));
                                m_block.at<Vec3b>(a,b)=nv;

                            }
                        }
                        vector<uchar> ff;
                        vector<uchar> noheader;
                        // use OpenCV's inbuilt JPEG encoder
                        imencode(".jpg",m_block,ff);

                        block.block_size = ff.size();
                        // strip JPEG header and keep it for future use
                        if (bheader.size()==0) {
                            for (int i=0;i<ff.size()-2;i++){
                                if (ff[i]==255 && ff[i+1]==218) {
                                    common_header_size=i;
                                    break;
                                } else {
                                    bheader.push_back(ff[i]);
                                }
                            }
                            if (common_header_size ==0 ){
                                cerr<<"Warning: cant strip JPEG header"<<endl;
                            }
                        }

                        for (int i=common_header_size+2;i<ff.size();i++) {
                            noheader.push_back(ff[i]);
                        }

                        block.data = noheader;
                        changed = true;
                        modd[i][j] = IPVC_DIFF;

                        l_blocks.push_back(block);
                    }

                }
                // draw overlay
                if (changed && parent->getIfOverlayVideoShown()) {
                    if (modd[i][j] == IPVC_DIFF) { //purple
                        for (int a = 0; a < BLOCK_SIZE; a++) {
                            for (int b = 0; b < BLOCK_SIZE; b++) {
                                m_output_overlay.at<Vec3b > (i * BLOCK_SIZE + a, j * BLOCK_SIZE + b) = Vec3b(150, 0, 156);
                            }
                        }
                    } else if (modd[i][j] == IPVC_PC) { //blue
                        for (int a = 0; a < BLOCK_SIZE; a++) {
                            for (int b = 0; b < BLOCK_SIZE; b++) {
                                m_output_overlay.at<Vec3b > (i * BLOCK_SIZE + a, j * BLOCK_SIZE + b) = Vec3b(250, 0, 0);
                            }
                        }
                    } else {
                        for (int a = 0; a < BLOCK_SIZE; a++) { //yellow
                            for (int b = 0; b < BLOCK_SIZE; b++) {
                                m_output_overlay.at<Vec3b > (i * BLOCK_SIZE + a, j * BLOCK_SIZE + b) = Vec3b(0, 255, 250);
                            }
                        }
                    }
                }
            }
        }

        // write block moves and new blocks to file
        if (!l_blocks.empty() || !l_block_moves.empty()) {

            ipvc_frame_header_t frh;
            frh.frame_type = 122;
            frh.frame_id = current_frame;
            frh.blocks = l_blocks.size();

            frh.block_moves = l_block_moves.size();

            fwrite(&frh, 1, sizeof (ipvc_frame_header_t), ipvc_file);
            frameSizeWithoutJpeg+=sizeof(frh);
            frameSize+=sizeof(frh);
            if (!l_blocks.empty()) {
                if (!common_header_written) {
                    ipvc_block_header_read_t hb;
                    hb.block_header_size=(ushort)bheader.size();

                    fwrite(&hb,1,sizeof(ipvc_block_header_read_t), ipvc_file);
                    frameSize+=sizeof(ipvc_block_header_read_t);

                    fwrite((uchar*)bheader.data(),1,bheader.size(),ipvc_file);
                    frameSize+=bheader.size();

                    common_header_written=true;
                }

                for (list<ipvc_block_t>::iterator it = l_blocks.begin(); it != l_blocks.end(); it++) {

                    ipvc_block_t bb = *it;
                    ipvc_block_read_t br;
                    br.block_id = bb.block_id;
                    br.block_size = bb.block_size;

                    fwrite(&br, 1, sizeof (ipvc_block_read_t), ipvc_file);
                    fwrite((uchar*)bb.data.data(), 1, bb.data.size(), ipvc_file);

                    frameSizeWithoutJpeg+=sizeof (ipvc_block_read_t)+BLOCK_SIZE*BLOCK_SIZE*3;
                    frameSize+=sizeof (ipvc_block_read_t)+bb.data.size();

                }
            }
            if (!l_block_moves.empty()) {
                for (list<ipvc_block_move_t>::iterator it = l_block_moves.begin(); it != l_block_moves.end(); it++) {
                    ipvc_block_move_t bm = *it;

                    fwrite(&bm, 1, sizeof (ipvc_block_move_t), ipvc_file);

                    frameSizeWithoutJpeg+=sizeof (ipvc_block_move_t);
                    frameSize+=sizeof (ipvc_block_move_t);

                }
            }
            l_blocks.clear();
            l_block_moves.clear();
        }
        // add overlay on top of the output image
        addWeighted(m_output, 1.0, m_output_overlay, 0.7, 0.0, m_final);
        if(parent->getIfOriginalVideoShown())
            imshow("Original_Video", frame);
        if(parent->getIfOutputVideoShown())
            imshow("Overlay_Video", m_final);
        if(parent->getIfOverlayVideoShown())
            imshow("Output_Video", m_output);

        waitKey(1);

        capture >> frame;
        current_frame++;

        // frame switching
        if (image == &m_image1) {
            image = &m_image2;
            previmage = &m_image1;
            greys_f = &m_output_greys_f2[0][0];
            prevgreys_f = &m_output_greys_f1[0][0];
        } else {
            image = &m_image1;
            previmage = &m_image2;
            greys_f = &m_output_greys_f1[0][0];
            prevgreys_f = &m_output_greys_f2[0][0];
        }
        parent->drawChart((frameSize/ftotal),frameSizeWithoutJpeg/ftotal);
        parent->calcPercentage(ftotal,frameSize);

        frameSizeWithoutJpeg=0;
        frameSize=0;
    }
    cerr << "Encoding finished" << endl;
    fflush(ipvc_file);
    fclose(ipvc_file);
    if(!parent->getChartState()){
        destroyWindow("Original_Video");
        destroyWindow("Overlay_Video");
        destroyWindow("Output_Video");
    }
}
