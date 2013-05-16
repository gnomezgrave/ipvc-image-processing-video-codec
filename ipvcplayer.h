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

#include <QApplication>
#include <QMainWindow>
#include <QFileDialog>
#include <QString>

namespace Ui {
class ipvcPlayer;
}

class ipvcPlayer : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit ipvcPlayer(QWidget *parent = 0);
    ~ipvcPlayer();
    
private slots:
    void on_browse_clicked();
    void readFile(QString file);

private:
    Ui::ipvcPlayer *ui;
};

#endif // IPVCPLAYER_H
