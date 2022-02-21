#include "uppersensor.h"
#include "ui_uppersensor.h"

UpperSensor::UpperSensor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpperSensor)
{
    ui->setupUi(this);
}

UpperSensor::~UpperSensor()
{
    delete ui;
}
