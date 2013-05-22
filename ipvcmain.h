#ifndef IPVCMAIN_H
#define IPVCMAIN_H

#include <QDialog>
#include <QGraphicsScene>

namespace Ui {
class IpvcMain;
}

class IpvcMain : public QDialog
{
    Q_OBJECT
    
public:
    explicit IpvcMain(QWidget *parent = 0);
    ~IpvcMain();
    void drawChart(qreal y, qreal wj);
public slots:
    void on_btnPlayer_clicked();
    void on_btnEncoder_clicked();
    void on_btnExit_clicked();
    bool getIfOriginalVideoShown();
    bool getIfOverlayVideoShown();
    bool getIfOutputVideoShown();
    void calcPercentage(double fullSize,double currentSize);
    bool getChartState();

private slots:
    void on_chkCharts_clicked();

private:
    QGraphicsScene gs;
    qreal prev_x;
    qreal prev_y;
    qreal prev_wj;
    double totalFullSize;
    double totalSize;
    int time;
    void setEnableControls(bool state);
    Ui::IpvcMain *ui;
};

#endif // IPVCMAIN_H
