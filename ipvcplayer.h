#ifndef IPVCPLAYER_H
#define IPVCPLAYER_H

#include <iostream>
#include <stdio.h>
#include <list>
#include <cstdio>
#include <cmath>
#include <stdint.h>
#include "ipvc.h"
#include <opencv2/highgui/highgui.hpp>

#include <QFileDialog>
#include <QString>
#include <QDialog>

namespace Ui {
class IpvcPlayer;
}

class IpvcPlayer
{

public:
    IpvcPlayer(char *inputfile);

};

#endif // IPVCPLAYER_H
