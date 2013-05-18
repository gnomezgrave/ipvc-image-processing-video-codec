#ifndef IPVC_H
#define IPVC_H

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
};

struct __attribute__((packed)) ipvc_frame_header_t {
    uchar frame_type; // 122: blocks
    unsigned frame_id;
    ushort blocks;
    ushort block_moves;
};
/*
struct __attribute__((packed)) ipvc_frame_full_header_read_t {
};*/

struct __attribute__((packed)) ipvc_frame_header_read_t {
    ushort blocks;
    ushort block_moves;
};

struct __attribute__((packed)) ipvc_block_t {
    ushort block_id;
    uchar data[BLOCK_SIZE * BLOCK_SIZE * 3];
};

struct __attribute__((packed)) ipvc_block_move_t {
    ushort block_id;
    float move_h;
    float move_w;
};

#endif // IPVC_H
