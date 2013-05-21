#ifndef IPVC_H
#define IPVC_H

// use block sizes <140
#define BLOCK_SIZE 20

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

struct __attribute__((packed)) ipvc_file_header_t {
    unsigned height;
    unsigned width;
    unsigned short block_size;
    unsigned char rate;
};

struct __attribute__((packed)) ipvc_frame_full_header_t {
    uchar frame_type; // 126: full frame
    unsigned frame_id;
    unsigned frame_size;
};

struct __attribute__((packed)) ipvc_frame_header_t {
    uchar frame_type; // 122: blocks
    // there will be
    // ushort block_header_size;
    // uchar block_header_data[];
    // here on the first block thats being read
    unsigned frame_id;
    ushort blocks;
    ushort block_moves;
};
struct __attribute__((packed)) ipvc_frame_full_header_read_t {
    unsigned frame_size;
};

struct __attribute__((packed)) ipvc_frame_header_read_t {
    ushort blocks;
    ushort block_moves;
};

struct __attribute__((packed)) ipvc_block_header_read_t {
    ushort block_header_size;
};

struct __attribute__((packed)) ipvc_block_read_t {
    ushort block_id;
    ushort block_size;
};

struct __attribute__((packed)) ipvc_block_t {
    ushort block_id;
    ushort block_size;
    std::vector<uchar> data;
};

struct __attribute__((packed)) ipvc_block_move_t {
    ushort block_id;
    float move_h;
    float move_w;
};

namespace cv{
Point2d phaseCorrelateX(InputArray _src1, InputArray _src2);
}

#endif // IPVC_H
