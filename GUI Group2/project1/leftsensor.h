#ifndef LEFTSENSOR_H
#define LEFTSENSOR_H

#include <QDialog>
#include <string.h>

namespace Ui {
class LeftSensor;
}

class LeftSensor : public QDialog
{
    Q_OBJECT

public:
    explicit LeftSensor(QWidget *parent = 0);
    ~LeftSensor();

private:
    Ui::LeftSensor *ui;
};

#endif // LEFTSENSOR_H
