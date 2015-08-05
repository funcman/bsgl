#include "MainTask.h"

#include <QCoreApplication>
#include <QPainter>

#include "MainWindow.h"
#include "ScreenWidget.h"
#include "bsgl_impl.h"

extern void bsgl_main();

MainTask::MainTask(QObject* parent)
:   QObject(parent) {
    isRunning = false;
}

MainTask::~MainTask() {
}

void MainTask::run() {
    ScreenWidget* screen = ScreenWidget::instance();
    isRunning = true;
    bsgl_main();
    emit finished();
}

void MainTask::quit() {
    isRunning = false;
}
