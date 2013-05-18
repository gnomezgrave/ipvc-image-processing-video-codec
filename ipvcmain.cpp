#include <QMessageBox>
#include "ipvcmain.h"
#include "ipvcplayer.h"
#include "ipvcencoder.h"
#include "ui_ipvcmain.h"

IpvcMain::IpvcMain(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IpvcMain)
{
    ui->setupUi(this);
}
void IpvcMain::on_btnPlayer_clicked()
{
    QString in = QFileDialog::getOpenFileName(this,"Select a file to play","./", "ipvc Files (*.ipvc)");
    IpvcPlayer ip(in.toLocal8Bit().data());
}
void IpvcMain::on_btnEncoder_clicked()
{
    QString in = QFileDialog::getOpenFileName(this,"Select a file to encode","./", "video Files (*.*)");
    //QString out = QFileDialog::getExistingDirectory(this, tr("Open Directory"),"./",QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);
    if(!in.isNull()){
        QString out = QFileDialog::getOpenFileName(this,"Select the path for the output file and type a name","./", "ipvc Files (*.ipvc)");
        if(!out.endsWith(".ipvc"))  out.append(".ipvc");
        while(FILE *f=fopen(out.toStdString().c_str(),"r")){
            int res=QMessageBox::information(this,"File already exists.","There is another file exists with the name you specified. Do you want to overwrite it?","Yes","No",0);
            if(res!=0){
                out = QFileDialog::getOpenFileName(this,"Select the path for the output file and type a name","./", "ipvc Files (*.ipvc)");
                if(!out.endsWith(".ipvc"))  out.append(".ipvc");
            }else{
               break;
            }
            fclose(f);
        }
        IpvcEncoder ie(in.toLocal8Bit().data(),out.toLocal8Bit().data());
    }

}
void IpvcMain::on_btnExit_clicked()
{

}
IpvcMain::~IpvcMain()
{
    delete ui;
}
