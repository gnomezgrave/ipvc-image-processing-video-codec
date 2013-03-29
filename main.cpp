/**
    This is my solution for a few problems that we will be facing for implementing a video codec.
    Those problems are,
        1. Most codecs use motion vectors, and we can't because we don't have the knowledge
        and its out of syllabus.
        2. It's hard to achieve the compression levels gained from motion vectors without
        using them.
        3. Will madam compare our codec with other codecs?

    This solution uses the differences between two frames and keeps adding it so the image is complete.
    The details that are removed will be compensated by blurring, it does a edge detection of the generated
    image and blurs out the places of the image that are not near the edges, so the edges keep sharpened
    and image quality is preseved to some level.

    The file will be a normal compressed file but what will be compressed using lzma(didnt add that yet)
    with 0s in the unchanged pixels the compression ratio will be high, we will control how much of changed
    pixels go in to the file as data. This can be done by changing the variable 'quality'.

    We have to decide on compressing every frame individually or compressing the whole file.

    Can someone add lzma to this?

    The terminal output is
    <percentage of kept pixels> <level used for cutting off pixels>

*/

#include <iostream>
#include <cstdio>
#include <cmath>
#include <stdint.h>
#include <lzma.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#define OUT_FILE "/home/madura/out.ipv"

#define PIXEL(jump, bigj, smallj) (((jump)*(bigj) + (smallj))*3)
#define ARR2D(jump, bigj, smallj) (((jump)*(bigj) + (smallj)))

#define SET(dest, src, at)   \
    dest[at]=src[at]; \
    dest[at+1]=src[at+1]; \
    dest[at+2]=src[at+2];

#define SET_EXT(dest, src, at1, at2)   \
    dest[at1]=src[at2]; \
    dest[at1+1]=src[at2+1]; \
    dest[at1+2]=src[at2+2];

#define AVG(a,b,c) (((a)+(b)+(c))/3.0)

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
struct ipvc_file_header_t{
    unsigned height;
    unsigned width;
};

struct ipvc_frame_header_t{
    unsigned length;
};
using namespace cv;
using namespace std;

int mask[3] = {-1,0,1};

int blurmask3[3][3] = { { 1, 2, 1 } ,
                        { 2, 4, 2 } ,
                        { 1, 2, 1 }
                      };

void divComplex(InputArray _src1, InputArray _src2, OutputArray _dst)
{
    Mat src1 = _src1.getMat();
    Mat src2 = _src2.getMat();

    CV_Assert( src1.type() == src2.type() && src1.size() == src2.size());
    CV_Assert( src1.type() == CV_32FC2 || src1.type() == CV_64FC2 );

    _dst.create(src1.size(), src1.type());
    Mat dst  = _dst.getMat();

    int length = src1.rows*src1.cols;

    if(src1.depth() == CV_32F)
    {
        const float* dataA = (const float*)src1.data;
        const float* dataB = (const float*)src2.data;
        float* dataC = (float*)dst.data;
        float eps = FLT_EPSILON; // prevent div0 problems

        for(int j = 0; j < length - 1; j += 2)
        {
            double denom = (double)(dataB[j]*dataB[j] + dataB[j+1]*dataB[j+1] + eps);
            double re = (double)(dataA[j]*dataB[j] + dataA[j+1]*dataB[j+1]);
            double im = (double)(dataA[j+1]*dataB[j] - dataA[j]*dataB[j+1]);
            dataC[j] = (float)(re / denom);
            dataC[j+1] = (float)(im / denom);
        }
    }
    else
    {
        const double* dataA = (const double*)src1.data;
        const double* dataB = (const double*)src2.data;
        double* dataC = (double*)dst.data;
        double eps = DBL_EPSILON; // prevent div0 problems

        for(int j = 0; j < length - 1; j += 2)
        {
            double denom = dataB[j]*dataB[j] + dataB[j+1]*dataB[j+1] + eps;
            double re = dataA[j]*dataB[j] + dataA[j+1]*dataB[j+1];
            double im = dataA[j+1]*dataB[j] - dataA[j]*dataB[j+1];
            dataC[j] = re / denom;
            dataC[j+1] = im / denom;
        }
    }
}

void absComplex(InputArray _src, OutputArray _dst)
{
    Mat src = _src.getMat();

    CV_Assert( src.type() == CV_32FC2 || src.type() == CV_64FC2 );

    vector<Mat> planes;
    split(src, planes);

    magnitude(planes[0], planes[1], planes[0]);
    planes[1] = Mat::zeros(planes[0].size(), planes[0].type());

    merge(planes, _dst);
}
Point phaseCorrelate2(InputArray _src1, InputArray _src2)
{

    Mat FFT1, FFT2, P, Pm, C;

    FFT1=_src1.getMat();
    FFT2=_src2.getMat();


    mulSpectrums(FFT1, FFT2, P, 0, true);


    absComplex(P, Pm);
     divComplex(P, Pm, C); // FF* / |FF*| (phase correlation equation completed here...)

     idft(C, C); // gives us the nice peak shift location...

     vector<Mat> Cplanes;
     split(C, Cplanes);
     C = Cplanes[0]; // use only the real plane since that's all that's left...

     //fftShift(C); // shift the energy to the center of the frame.

     // locate the highest peak
     Point peakLoc;
     minMaxLoc(C, NULL, NULL, NULL, &peakLoc);
     if (peakLoc.x> FFT1.cols/2)
         peakLoc.x -=FFT1.cols;
     if (peakLoc.y> FFT1.rows/2)
         peakLoc.y -=FFT1.rows;
    // C.at<float>(peakLoc.y,peakLoc.x) = 0.0f;
    // minMaxLoc(C, NULL, NULL, NULL, &peakLoc);
     return peakLoc;
 }
int main(int argc, char *argv[])
{
    uint32_t preset = COMPRESSION_LEVEL | (COMPRESSION_EXTREME ? LZMA_PRESET_EXTREME : 0);
    lzma_check check = INTEGRITY_CHECK;
    lzma_stream strm = LZMA_STREAM_INIT; /* alloc and init lzma_stream struct */
    uint8_t *in_buf;
    uint8_t *out_buf;
    size_t in_len;	/* length of useful data in in_buf */
    size_t out_len;	/* length of useful data in out_buf */
    bool in_finished = false;
    bool out_finished = false;
    lzma_action action;
    lzma_ret ret_xz;
    char press=-1;
    float quality=0.4f; // number between 0 and 1, can't be 0

    unsigned changes;

    FILE *ipvc_file = fopen(OUT_FILE, "w");

    namedWindow( "video_ori", CV_WINDOW_AUTOSIZE );
    namedWindow( "video_out", CV_WINDOW_AUTOSIZE );
    namedWindow( "video_grey", CV_WINDOW_AUTOSIZE );

    VideoCapture capture;
    capture.open( argv[1]);
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

    Mat m_image1(height, width, CV_8UC3);
    Mat m_image2(height, width, CV_8UC3);
    Mat m_output(height, width, CV_8UC3);
    Mat m_output_grey(height,width, CV_8UC1);
    Mat m_output_grey_f(height,width, CV_32FC1);
    Mat m_fft1(height,width, CV_32FC2);
    Mat m_fft2(height,width, CV_32FC2);
    Mat m_cross_power(height,width, CV_32FC1);
    Mat m_blurred(height,width, CV_8UC1);
    Mat m_fft_op_re(height,width, CV_32FC1);
    Mat m_fft_op_img(height,width, CV_32FC1);
    Mat m_fft_op_p(height,width, CV_32FC2);
    Mat m_fft_op_pm(height,width, CV_32FC2);

    Mat m_fft_planes[]={m_fft_op_re,m_fft_op_img};
    Mat m_fft_op_div(height,width, CV_32FC1);

    Mat m_changed1(height,width, CV_32SC1);
    Mat m_changed2(height,width, CV_32SC1);

    Mat m_translate(2,3,CV_32FC1);
    m_translate.at<float>(0,0) = 1;
    m_translate.at<float>(0,1) = 0;
    m_translate.at<float>(1,0) = 0;
    m_translate.at<float>(1,1) = 1;

m_translate.at<float>(0,2)=0;
m_translate.at<float>(1,2)=0;


    Mat *fft=&m_fft1;
    Mat *prevfft=&m_fft2;
    Mat *image=NULL,*previmage=NULL, *blurred,*changed,*prevchanged;
    Mat *output,*output_grey;

    image=&m_image1;
    previmage=&m_image2;
    output=&m_output;
    output_grey=&m_output_grey;
    blurred=&m_blurred;
    changed=&m_changed1;
    prevchanged=&m_changed2;

    in_buf = (uint8_t *) m_output.data;
    out_buf = (uint8_t *) malloc(height*width*sizeof(uchar)*3);

    for (unsigned i=0;i<height;i++){
        for (unsigned j=0;j<width;j++) {
            previmage->at<uchar>(i,j) = 0;
            blurred->at<uchar>(i,j)= 1;
            output->at<Vec3b>(i,j)=Vec3b(0,0,0);
            prevfft->at<Vec2f>(i,j)=Vec2f(0.0f,0.0f);
            fft->at<Vec2f>(i,j)=Vec2f(0.0f,0.0f);
            changed->at<int>(i,j)=0;
            prevchanged->at<int>(i,j)=0;
        }
    }

    /* initialize xz encoder */
    ret_xz = lzma_easy_encoder (&strm, preset, check);
    if (ret_xz != LZMA_OK) {
        std::cerr <<"lzma_easy_encoder error: "<< ret_xz<<std::endl;
        return -1;
    }

    ipvc_file_header_t fh;
    ipvc_frame_header_t frh;
    fh.height = height;
    fh.width = width;

    fwrite(&fh, 1,sizeof(ipvc_file_header_t),ipvc_file);
    std::cout<<"sd"<<endl;

    cvtColor(frame,m_output_grey,CV_BGR2GRAY);
    m_output_grey.convertTo(m_output_grey_f,CV_32FC1);
    dft(m_output_grey_f, *prevfft,DFT_COMPLEX_OUTPUT);

    capture >> frame;
    float avg=0.0f;
    while(!frame.empty() && press==-1) {
        //uncomment this to see what a frame actually has
       m_output = Mat::zeros(height, width, CV_8UC3);

        cvtColor(frame,m_output_grey,CV_BGR2GRAY);
        m_output_grey.convertTo(m_output_grey_f,CV_32FC1);
        dft(m_output_grey_f, *fft,DFT_COMPLEX_OUTPUT);


        Point pc=phaseCorrelate2(*prevfft,*fft);
        m_translate.at<float>(0,2) += pc.x;
        m_translate.at<float>(1,2) += pc.y;

        warpAffine(frame,*image,m_translate,Size(width,height));

        unsigned differences=0;
        for (unsigned i=0;i<height;i++){
            for (unsigned j=0;j<width;j++) {
                if (image->at<Vec3b>(i,j)!=previmage->at<Vec3b>(i,j))
                    differences++;
            }
        }
        int level=(total-differences)/(ftotal*quality);

        changes=0;
        for (unsigned i=0;i<height;i++){
            for (unsigned j=0;j<width;j++) {
                changed->at<int>(i,j)=0;
                if (abs(AVG(image->at<Vec3b>(i,j)[0]-previmage->at<Vec3b>(i,j)[0],
                            image->at<Vec3b>(i,j)[1]-previmage->at<Vec3b>(i,j)[1],
                            image->at<Vec3b>(i,j)[2]-previmage->at<Vec3b>(i,j)[2]
                            )) > level){
                    output->at<Vec3b>(i,j)[0] = image->at<Vec3b>(i,j)[0];
                    output->at<Vec3b>(i,j)[1] = image->at<Vec3b>(i,j)[1];
                    output->at<Vec3b>(i,j)[2] = image->at<Vec3b>(i,j)[2];

                    changed->at<int>(i,j)=1;
                    changes++;
                }
            }
        }
        avg=(avg+changes/ftotal)/2.0f;
        cout<<avg<<" "<<level<<endl;
        cvtColor(m_output,m_output_grey,CV_BGR2GRAY);
        Canny(m_output_grey,m_output_grey,100,255);
        for (unsigned i=1;i<height-1;i++){
            for (unsigned j=1;j<width-1;j++) {
                if (prevchanged->at<int>(i,j)==1 &&
                        m_output_grey.at<uchar>(i-1,j-1) +
                        m_output_grey.at<uchar>(i+1,j-1) +
                        m_output_grey.at<uchar>(i+1,j+1) +
                        m_output_grey.at<uchar>(i-1,j+1) +
                        m_output_grey.at<uchar>(i,j)



                        >0
                        ) {
                    Vec3b g(0,0,0);
                    for (int mi=0;mi<3;mi++) {
                        for (int mj=0;mj<3;mj++) {
                            g += output->at<Vec3b>(i+mask[mi],j+mask[mj])*(blurmask3[mj][mi]/16.0 );

                        }
                    }
                    output->at<Vec3b>(i,j) = g;
                    prevchanged->at<int>(i,j)=0;
                }

            }
        }

      //  cout<<1.0f-changes*1.0f/total<<" compressed"<<endl;
      // / imshow("video_ori", frame);
       // imshow("video_out", m_output);
       // imshow("video_grey", m_output_grey);
        press= waitKey(1);

        strm.next_in = in_buf;
        strm.avail_in = total_size;
        strm.next_out = out_buf;
        strm.avail_out = total_size;

        /* compress data */
        ret_xz = lzma_code (&strm, LZMA_SYNC_FLUSH);
        if ((ret_xz != LZMA_OK) && (ret_xz != LZMA_STREAM_END)) {
            std::cerr<<"compression error"<<std::endl;
        } else {

            out_len = total_size - strm.avail_out;
            frh.length = out_len;
            fwrite(&frh,1,sizeof(ipvc_frame_header_t),ipvc_file);
            fwrite(out_buf,1,out_len,ipvc_file);
            std::cout<<out_len*1.0/total_size<<std::endl;
        }

        capture>>frame;
        if (image==&m_image1){
            image=&m_image2;
            previmage=&m_image1;
            fft=&m_fft2;
            prevfft=&m_fft1;
            changed=&m_changed2;
            prevchanged=&m_changed1;
        } else {
            image=&m_image1;
            previmage=&m_image2;
            fft=&m_fft1;
            prevfft=&m_fft2;
            changed=&m_changed1;
            prevchanged=&m_changed2;
        }


    }
    cout<<total_size<<endl;
    fclose(ipvc_file);
    lzma_end (&strm);
    return 0;
}

