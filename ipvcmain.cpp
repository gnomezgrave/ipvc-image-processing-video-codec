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
    IpvcPlayer ip("/home/madura/out.ipvc");
}
void IpvcMain::on_btnEncoder_clicked()
{
    IpvcEncoder ie("/home/madura/Desktop/ss.webm","/home/madura/out.ipvc");

}
void IpvcMain::on_btnExit_clicked()
{

}
IpvcMain::~IpvcMain()
{
    delete ui;
}
