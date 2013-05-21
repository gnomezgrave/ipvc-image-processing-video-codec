#ifndef IPVCENCODER_H
#define IPVCENCODER_H
#include "ipvcplayer.h"
#include "ipvcmain.h"

class IpvcEncoder
{
private:
    int frameSize;
    int frameSizeWithoutJpeg;
public:
    IpvcEncoder(IpvcMain* parent,QString inputfile, QString outputfile);
};

#endif // IPVCENCODER_H
