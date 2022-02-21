#include "rightsensor.h"
#include "ui_rightsensor.h"

RightSensor::RightSensor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RightSensor)
{
    ui->setupUi(this);

    scene = new QGraphicsScene(this);
    ui->graphicsView ->setScene(scene);

    QBrush redBrush (Qt::red);
    QBrush greenBrush (Qt::green);
    QBrush blueBrush (Qt::blue);

    QPen blackpen (Qt::black);
    blackpen.setWidth(5);
}

RightSensor::~RightSensor()
{
    delete ui;
}

