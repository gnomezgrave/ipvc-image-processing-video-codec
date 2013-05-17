#include <QApplication>
#include "ipvcmain.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    IpvcMain ip;
    ip.show();
    return a.exec();
}
