#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#define WIDTH 6

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setCentralWidget(scroll);
    scroll->setWidget(w);
    scroll->setMinimumSize(940,500);
    scroll->setWidgetResizable(true);
    w->setLayout(grid);
    w->setMinimumSize(940,4500);

    QLabel *lbl = new QLabel(" ");
    grid->addWidget(lbl,0,0,1,17);
    QPushButton *btn = new QPushButton("get tree");
    connect(btn, SIGNAL(clicked()), this, SLOT(getTreeButtonClicked()));
    grid->addWidget(btn,1,0,Qt::AlignTop);
    grid->setAlignment(Qt::AlignTop);
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::getTreeButtonClicked()
{
    connect_module("tree");
}

void MainWindow::list()
{
    //remove 'get tree' button
    QLayoutItem *to_delete = grid->takeAt(grid->indexOf(grid->itemAtPosition(1,0)->widget()));
    delete to_delete->widget();
    delete to_delete;

    for(int i = 0; i < lines.size(); i++)
    {
        QPushButton *plusbtn = new QPushButton("+");
        plusbtn->setMaximumSize(QSize(20,20));
        plusbtn->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);
        plus_buttons.push_back(plusbtn);
        connect(plusbtn, SIGNAL(clicked()), this, SLOT(plusButtonClicked()));

        QPushButton *flbtn = new QPushButton("files");
        flbtn->setMaximumSize(QSize(30,20));
        flbtn->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);
        fl_buttons.push_back(flbtn);
        connect(flbtn, SIGNAL(clicked()), this, SLOT(fileButtonClicked()));

        QPushButton *membtn = new QPushButton("memory");
        membtn->setMaximumSize(QSize(60,20));
        membtn->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);
        mem_buttons.push_back(membtn);
        connect(membtn, SIGNAL(clicked()), this, SLOT(memButtonClicked()));

        visible.push_back(false);

        if(lines[i].compare(0,1," "))
        {
            grid->addWidget(plusbtn,i+1,0,1,1,Qt::AlignTop);
            QLabel *text = new QLabel(QString::fromUtf8(lines[i].c_str()));
            grid->addWidget(text,i+1,1,1,WIDTH,Qt::AlignTop);
            visible[i] = true;
        }
    }
}

int MainWindow::connect_module(std::string msg)
{
    std::ofstream fw;
    fw.open("/proc/module_proc_file");
    if(fw.is_open())
    {
        fw << msg;
        fw.close();
    }
    else
    {
        ui->statusBar->showMessage("module is not loaded");
        return 0;
    }

    std::ifstream fr("/proc/module_proc_file");
    if(fr.is_open())
    {
        if(msg=="tree")
        {
            std::string line;
            while (std::getline(fr, line))
                lines.push_back(line);
            fr.close();
            list();
        }
        if(msg.compare(0,3,"mem") == 0 || msg.compare(0,3,"fls") == 0)
        {
            QMainWindow *new_window = new QMainWindow();
            QScrollArea *new_scroll = new QScrollArea();
            new_window->setCentralWidget(new_scroll);
            QWidget *new_w = new QWidget();
            new_scroll->setWidget(new_w);
            QVBoxLayout *vbox = new QVBoxLayout();
            new_w->setLayout(vbox);
            new_scroll->setMinimumSize(420,500);
            new_scroll->setWidgetResizable(true);
            new_w->setMinimumSize(400,12000);
            vbox->setAlignment(Qt::AlignTop);

            std::string line;
            while (std::getline(fr, line))
            {
                QLabel *lbl = new QLabel();
                lbl->setText(QString::fromUtf8(line.c_str()));
                vbox->addWidget(lbl);
            }
            fr.close();

            new_window->show();
        }
    }
}

void MainWindow::plusButtonClicked()
{
    QPushButton *button = (QPushButton *)sender();
    std::vector<QPushButton*>::iterator it;
    it = std::find(plus_buttons.begin(),plus_buttons.end(),button);
    int start = it-plus_buttons.begin(); // index of clicked button in button vector

    if((*it)->text() == "+") // unfold
    {
        (*it)->setText("-");

        int level = 0; // parent (clicked button) level
        while(lines[start][level] == ' ')
            level++;
        for(int i = start+1; i < lines.size(); i++)
        {
            int level2 = 0; // child level
            while(lines[i][level2] == ' ')
                level2++;
            if(level2 <= level)
                break; // no more children of this parent
            if(level2-level > 1)
                continue; // yet hidden children

            QLabel *text = new QLabel(QString::fromUtf8(lines[i].substr(level2,lines[i].size()-level2).c_str()));
            int level3 = 0; // grandchild level (to check are there any)
            if(i != lines.size()-1)
            {
                while(lines[i+1][level3] == ' ')
                    level3++;
                if(level3 > level2)
                {
                    grid->addWidget(plus_buttons[i],i+1,level2,1,1,Qt::AlignTop);
                    plus_buttons[i]->setVisible(1);
                }
            }
            grid->addWidget(text,i+1,level2+1,1,WIDTH,Qt::AlignTop);
            grid->addWidget(mem_buttons[i],i+1,level2+WIDTH+2,1,1,Qt::AlignTop);
            mem_buttons[i]->setVisible(1);
            grid->addWidget(fl_buttons[i],i+1,level2+WIDTH+3,1,1,Qt::AlignTop);
            fl_buttons[i]->setVisible(1);
            visible[i] = true;
        }
    }
    else // fold back
    {
        (*it)->setText("+");

        int level = 0; // parent (clicked button) level
        while(lines[start][level] == ' ')
            level++;
        for(int i = start+1; i < lines.size(); i++)
        {
            int level2 = 0; // child level
            while(lines[i][level2] == ' ')
                level2++;
            if(level2 <= level)
                break; // no more children of this parent

            if(visible[i])
            {
                if(grid->itemAtPosition(i+1,level2))
                {
                    QLayoutItem *to_delete = grid->takeAt(grid->indexOf(grid->itemAtPosition(i+1,level2)->widget()));
                    to_delete->widget()->setVisible(0);
                    delete to_delete;
                }
                QLayoutItem *text_delete = grid->takeAt(grid->indexOf(grid->itemAtPosition(i+1,level2+1)->widget()));
                text_delete->widget()->setVisible(0);
                delete text_delete;
                QLayoutItem *mem_delete = grid->takeAt(grid->indexOf(grid->itemAtPosition(i+1,level2+WIDTH+2)->widget()));
                mem_delete->widget()->setVisible(0);
                delete mem_delete;
                QLayoutItem *fls_delete = grid->takeAt(grid->indexOf(grid->itemAtPosition(i+1,level2+WIDTH+3)->widget()));
                fls_delete->widget()->setVisible(0);
                delete fls_delete;
                visible[i] = false;
            }
        }
    }
}

void MainWindow::fileButtonClicked()
{
    QPushButton *button = (QPushButton *)sender();
    std::vector<QPushButton*>::iterator it;
    it = std::find(fl_buttons.begin(),fl_buttons.end(),button);

    int index = it-fl_buttons.begin(); // index of clicked button in button vector
    std::string pid = lines[index].substr(lines[index].find("[")+1, lines[index].find("]") - lines[index].find("[")-1);
    connect_module("fls"+pid);
}

void MainWindow::memButtonClicked()
{
    QPushButton *button = (QPushButton *)sender();
    std::vector<QPushButton*>::iterator it;
    it = std::find(mem_buttons.begin(),mem_buttons.end(),button);

    int index = it-mem_buttons.begin(); // index of clicked button in button vector
    std::string pid = lines[index].substr(lines[index].find("[")+1, lines[index].find("]") - lines[index].find("[")-1);
    connect_module("mem"+pid);
}
