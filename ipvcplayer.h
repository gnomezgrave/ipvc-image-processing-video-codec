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

class IpvcPlayer : public QDialog
{
    Q_OBJECT
    
public:
    explicit IpvcPlayer(QWidget *parent = 0);
    ~IpvcPlayer();
    
private slots:
    void on_browse_clicked();
    void readFile(QString file);

private:
    Ui::IpvcPlayer *ui;
};

#endif // IPVCPLAYER_H
