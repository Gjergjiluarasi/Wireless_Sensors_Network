#include "rightsensor"
#include "ui_newwindow.h"

newwindow::newwindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::newwindow)
{
    ui->setupUi(this);
}

newwindow::~newwindow()
{
    delete ui;
}
