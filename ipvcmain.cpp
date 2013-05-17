#include "ipvcmain.h"
#include "ipvcplayer.h"
#include "ipvcencoder.h"
#include "ui_ipvcmain.h"
#include <iostream>
IpvcMain::IpvcMain(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IpvcMain)
{
    ui->setupUi(this);
}
void IpvcMain::on_btnPlayer_clicked()
{
    IpvcPlayer ip;
    ip.setVisible(true);
    ip.exec();
}
void IpvcMain::on_btnEncoder_clicked()
{
    IpvcEncoder ie("/home/madura/Desktop/sx.mp4","/home/madura/out.ipvc");

}
void IpvcMain::on_btnExit_clicked()
{

}
IpvcMain::~IpvcMain()
{
    delete ui;
}
