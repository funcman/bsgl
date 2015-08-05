#ifndef SCREENWIDGET_H
#define SCREENWIDGET_H

#include <QtOpenGL>
#include <QWidget>

#include "bsgl.h"

class ScreenWidget : public QGLWidget
{
    Q_OBJECT

public:
    explicit ScreenWidget(QWidget* parent=0, int width=640, int height=480);
    ~ScreenWidget();

    static ScreenWidget* instance();

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    void keyPressEvent(QKeyEvent* e);
    void keyReleaseEvent(QKeyEvent* e);
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void focusOutEvent(QFocusEvent *e);

private:
    int     width_;
    int     height_;
};

#endif//SCREENWIDGET_H
