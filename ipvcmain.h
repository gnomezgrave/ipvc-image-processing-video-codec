#ifndef IPVCMAIN_H
#define IPVCMAIN_H

#include <QDialog>

namespace Ui {
class IpvcMain;
}

class IpvcMain : public QDialog
{
    Q_OBJECT
    
public:
    explicit IpvcMain(QWidget *parent = 0);
    ~IpvcMain();
public slots:
    void on_btnPlayer_clicked();
    void on_btnEncoder_clicked();
    void on_btnExit_clicked();

private:
    Ui::IpvcMain *ui;
};

#endif // IPVCMAIN_H
