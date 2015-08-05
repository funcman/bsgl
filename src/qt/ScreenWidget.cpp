#include "ScreenWidget.h"

#include <assert.h>
#include <QPainter>

#include "bsgl_impl.h"

extern void _InitOGL();
extern void _Resize(int, int);

static ScreenWidget* instance_ = 0;

ScreenWidget::ScreenWidget(QWidget* parent, int width, int height)
:   QGLWidget(parent)
,   width_(width)
,   height_(height) {
    instance_ = this;
}

ScreenWidget::~ScreenWidget() {
    instance_ = 0;
}

ScreenWidget* ScreenWidget::instance() {
    assert(instance_ != 0);
    return instance_;
}

void ScreenWidget::initializeGL() {
    setFocusPolicy(Qt::StrongFocus);
    setGeometry(0, 0, width_, height_);//设置窗口初始位置和大小
    _InitOGL();
}

void ScreenWidget::paintGL() {
}

void ScreenWidget::resizeGL(int width, int height) {
    _Resize(width, height);
}

void ScreenWidget::keyPressEvent(QKeyEvent* e) {
    if (!hasFocus()) {
        return;
    }
    if (e->key() == Qt::Key_F1)

    if (e->key() >= 0x0 && e->key() <= 0xFF) {
        qtKeyStates[e->key()] = true;
    }
}

void ScreenWidget::keyReleaseEvent(QKeyEvent* e) {
    if (e->key() >= 0x0 && e->key() <= 0xFF) {
        qtKeyStates[e->key()] = false;
    }
}

void ScreenWidget::mousePressEvent(QMouseEvent* e) {
    if (!hasFocus()) {
        return;
    }
    if (e->button() == Qt::LeftButton) {
        qtLeftButton = true;
    }
    if (e->button() == Qt::RightButton) {
        qtRightButton = true;
    }
}

void ScreenWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        qtLeftButton = false;
    }
    if (e->button() == Qt::RightButton) {
        qtRightButton = false;
    }
}

void ScreenWidget::focusOutEvent(QFocusEvent *e) {
    for (int i=0; i<0xFF; ++i) {
        qtKeyStates[i] = false;
    }
}
