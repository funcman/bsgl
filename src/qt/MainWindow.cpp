#include "MainWindow.h"
//#include "ui_MainWindow.h"

#include <assert.h>

#include "ScreenWidget.h"

static MainWindow* instance_ = 0;

MainWindow::MainWindow(QWidget* parent)
//:   QMainWindow(parent)
//,   ui(new Ui::MainWindow) {
:   QMainWindow(parent) {
    instance_ = this;

    setCentralWidget(new QWidget(this));

//    ui->setupUi(this);
}

MainWindow::~MainWindow() {
//    delete ui;
    instance_ = 0;
}

MainWindow* MainWindow::instance() {
    assert(instance_ != 0);
    return instance_;
}

void MainWindow::showEvent(QShowEvent* event) {
    QRect rect = centralWidget()->geometry();
    int dw = 320 - rect.width();
    int dh = 240 - rect.height();

    QWidget* w = new ScreenWidget(this, 320, 240);
    w->setGeometry(0, 0, 320, 240);
    setCentralWidget(w);

    rect.setWidth(geometry().width()+dw);
    rect.setHeight(geometry().height()+dh);
    this->setGeometry(rect);
    this->setFixedSize(rect.size());
}
