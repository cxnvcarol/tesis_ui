#include "simului.h"
#include <QApplication>
#include "glwidget.h"
#include "gllegendwidget.h"
#include <QtOpenGL/QGLFormat>
#include <QOpenGLContext>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    glutInit(&argc, argv);
    GLWidget w;
    //w.makeCurrent();
    w.setWindowTitle("Xplosion simulation");
    w.show();

    SimulUI wui;
    wui.setWindowTitle("Simulation dashboard");
    wui.show();
    wui.setSimulWindow(&w);
    wui.setSimulSystem(w.getSimulationSystem());
    wui.setDefaults();
    return a.exec();
}
