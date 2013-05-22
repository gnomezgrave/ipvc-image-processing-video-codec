#include "ipvcmain.h"
#include "ipvcplayer.h"
#include "ipvcencoder.h"
#include "ui_ipvcmain.h"
#include "ipvc.h"
#include <QtGui>
#include <QIcon>

#define MARGIN 10
#define CHART_HEIGHT 200

IpvcMain::IpvcMain(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IpvcMain)
{
    this->setWindowIcon(QIcon(":icon/res/Icon.ico"));
    ui->setupUi(this);
    time=0;

    double low=(ui->graphicsView->height()-CHART_HEIGHT)/2;

    prev_x=0;
    prev_y=CHART_HEIGHT+low;
    prev_wj=CHART_HEIGHT+low;
    ui->graphicsView->setScene(&gs);

    totalFullSize=0;
    totalSize=0;

}
void IpvcMain::on_btnPlayer_clicked()
{
    while(true){
        QString in = QFileDialog::getOpenFileName(this,"Select a file to play","./", "ipvc Files (*.ipvc)");
        if(in.isNull())continue;
        setEnableControls(false);
        IpvcPlayer ip(in);
        setEnableControls(true);
        break;
    }
}
void IpvcMain::on_btnEncoder_clicked()
{
    gs.clear();

    QString in = QFileDialog::getOpenFileName(this,"Select a file to encode","./", "video Files (*.*)");
    //QString out = QFileDialog::getExistingDirectory(this, tr("Open Directory"),"./",QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);
    QString out=NULL;
    if(!in.isNull()){

        out = QFileDialog::getSaveFileName(this,"Select the path for the output file and type a name","./", "ipvc Files (*.ipvc)");
        if(!out.endsWith(".ipvc"))
            out.append(".ipvc");

        if(QFile::exists(out)){

            setEnableControls(false);
            IpvcEncoder ie(this,in,out);
            setEnableControls(true);

            totalFullSize=totalSize=0;
        }
    }
}
void IpvcMain::setEnableControls(bool state){
    ui->chkCharts->setEnabled(state);
    ui->chkOri->setEnabled(state);
    ui->chkOvr->setEnabled(state);
    ui->chkOut->setEnabled(state);
    ui->btnEncoder->setEnabled(state);
    ui->btnPlayer->setEnabled(state);
}

void IpvcMain::on_btnExit_clicked()
{
    exit(0);
}
IpvcMain::~IpvcMain()
{
    delete ui;
}
bool IpvcMain::getIfOriginalVideoShown(){
    return ui->chkOri->isChecked();
}
bool IpvcMain::getIfOverlayVideoShown(){
    return ui->chkOvr->isChecked();
}
bool IpvcMain::getIfOutputVideoShown(){
    return ui->chkOut->isChecked();
}

void IpvcMain::drawChart(qreal y,qreal wj){

    if(ui->chkCharts->isChecked()){
        double low=(ui->graphicsView->height()-CHART_HEIGHT)/2;
        y*=CHART_HEIGHT;
        wj*=CHART_HEIGHT;

        time+=3;

        gs.addLine(prev_x,CHART_HEIGHT+low,time,CHART_HEIGHT+low,QPen(QColor(0,0,255,255)));

        gs.addLine(prev_x,prev_y,time,CHART_HEIGHT+low-y,QPen(QColor(255,0,0,255)));
        gs.addLine(prev_x,prev_wj,time,CHART_HEIGHT+low-wj,QPen(QColor(100,100,0,255)));

        gs.addLine(prev_x,0,time,0,QPen(QColor(0,255,0,255)));
        prev_x=time;
        prev_y=CHART_HEIGHT+low-y;
        prev_wj=CHART_HEIGHT+low-wj;
        gs.update();
    }
}
void IpvcMain::calcPercentage(double fullSize, double currentSize){
    totalFullSize+=fullSize;
    totalSize+=currentSize;
    double ans=totalFullSize==0?1:(totalSize/totalFullSize)*100;
    ui->perc->setText(QString::number(ans,'f',2).append("%"));
}

void IpvcMain::on_chkCharts_clicked()
{
    bool charts=!ui->chkCharts->isChecked();
    ui->chkOri->setEnabled(charts);
    ui->chkOut->setEnabled(charts);
    ui->chkOvr->setEnabled(charts);

    ui->chkOri->setChecked(charts);
    ui->chkOut->setChecked(charts);
    ui->chkOvr->setChecked(charts);
}
bool IpvcMain::getChartState(){
    return ui->chkCharts->isChecked();
}
