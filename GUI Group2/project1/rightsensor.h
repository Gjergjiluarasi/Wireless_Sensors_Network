#ifndef RIGHTSENSOR_H
#define RIGHTSENSOR_H

#include <QDialog>
#include <QtCore>
#include <QtGui>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <string.h>


namespace Ui {
class RightSensor;
}

class RightSensor : public QDialog
{
    Q_OBJECT

public:
    explicit RightSensor(QWidget *parent = 0);
    ~RightSensor();

//protected:
   // void changeEvent(QEvent *e);
   // void paintEvent(QPaintEvent *e);

private:
    Ui::RightSensor *ui;
    QGraphicsScene *scene;
    QGraphicsEllipseItem *ellipse;
    QGraphicsTextItem *text;

};

#endif // RIGHTSENSOR_H
