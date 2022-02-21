#include "leftsensor.h"
#include "ui_leftsensor.h"

LeftSensor::LeftSensor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LeftSensor)
{
    ui->setupUi(this);
}

LeftSensor::~LeftSensor()
{
    delete ui;
}
