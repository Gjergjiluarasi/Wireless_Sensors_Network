#ifndef UPPERSENSOR_H
#define UPPERSENSOR_H

#include <QDialog>
#include <string.h>

namespace Ui {
class UpperSensor;
}

class UpperSensor : public QDialog
{
    Q_OBJECT

public:
    explicit UpperSensor(QWidget *parent = 0);
    ~UpperSensor();

private:
    Ui::UpperSensor *ui;
};

#endif // UPPERSENSOR_H
