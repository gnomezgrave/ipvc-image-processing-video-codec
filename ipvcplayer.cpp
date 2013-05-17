#include "ipvcplayer.h"
#include "ui_ipvcplayer.h"

using namespace cv;
using namespace std;

IpvcPlayer::IpvcPlayer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IpvcPlayer)
{
    ui->setupUi(this);
}

IpvcPlayer::~IpvcPlayer()
{
    delete ui;
}

void IpvcPlayer::on_browse_clicked()
{
    QString s = QFileDialog::getOpenFileName(this,"Choose a file..." ,"./", "ipvc Files (*.ipvc)");
    if(!s.isNull()){
        ui->path->setText(s);
        this->readFile(s);
    }
}

void IpvcPlayer::readFile(QString file){

    namedWindow("Output", CV_WINDOW_AUTOSIZE );

    FILE *ipvc_file = fopen(file.toStdString().c_str(), "r");
    ipvc_file_header_t *fh = (ipvc_file_header_t*) malloc(sizeof(ipvc_file_header_t));
    fread(fh,1,sizeof(ipvc_file_header_t),ipvc_file);

    Mat output(fh->height,fh->width,CV_8UC3);

    char type;
    while(fread(&type,1,sizeof(char),ipvc_file)){
        if((char)type==126){
            for(int i=0;i<fh->height;i++){
                for(int j=0;j<fh->width;j++){
                    char r,g,b;
                    fread(&r,1,1,ipvc_file);
                    fread(&g,1,1,ipvc_file);
                    fread(&b,1,1,ipvc_file);
                    output.at<Vec3b>(i,j)=Vec3b(r,g,b);
                }
            }
            imshow("Output", output);

        }else if((char)type==122){
            printf("Move\n");
        }
    }

}

