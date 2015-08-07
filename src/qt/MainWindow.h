#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent=0);
    ~MainWindow();

    static MainWindow* instance();

    void showEvent(QShowEvent* event);

private:
//    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
