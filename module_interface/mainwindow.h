#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include <QPushButton>
#include <QGridLayout>
#include <QScrollArea>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void getTreeButtonClicked();
    void plusButtonClicked();
    void fileButtonClicked();
    void memButtonClicked();

private:
    Ui::MainWindow *ui;
    int connect_module(std::string msg);
    void list();

    QGridLayout *grid = new QGridLayout();
    QWidget *w = new QWidget();
    QScrollArea *scroll = new QScrollArea();

    std::vector<std::string> lines;
    std::vector<QPushButton*> plus_buttons;
    std::vector<QPushButton*> fl_buttons;
    std::vector<QPushButton*> mem_buttons;
    std::vector<bool> visible;
};

#endif // MAINWINDOW_H
