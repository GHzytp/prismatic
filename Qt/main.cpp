// Copyright Alan (AJ) Pryor, Jr. 2017
// Transcribed from MATLAB code by Colin Ophus
// PRISM is distributed under the GNU General Public License (GPL)
// If you use PRISM, we ask that you cite the following papers:

#define PRISM_BUILDING_GUI 1
#include <QApplication>
#include "prismmainwindow.h"
int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    //QFont font("Comic Sans MS");
    //font.setStyleHint(QFont::Monospace);
    //QApplication::setFont(font);
    QApplication a(argc, argv);
    PRISMMainWindow w;
    w.show();
    w.setGeometry(100,100,850,700);
    return a.exec();
}
