#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qdebug.h>
#include "rightsensor.h"
#include "leftsensor.h"
#include "uppersensor.h"


bool routes_drawing, segments_drawing,collision_drawing,segments_ordering;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // Get all available COM Ports and store them in a QList.
    //To poll the system for a list of connected devices, simply use getPorts(). Each QextPortInfo structure will populated with information about the corresponding device.
    QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();
    // Read each element on the list, but
    // add only USB ports to the combo box.
      for (int i = 0; i < ports.size(); i++) {
        if (ports.at(i).portName.contains("USB")){
            ui->comboBox_Interface->addItem(ports.at(i).portName.toLocal8Bit().constData());
        }
      }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}


void MainWindow::on_pushButton_clicked()
{
  RightSensor rightSensor;
  rightSensor.setModal(true);
  rightSensor.exec();
}
 

void MainWindow::on_pushButton_2_clicked()
{
   LeftSensor leftSensor;
   leftSensor.setModal(true);
   leftSensor.exec();
}

void MainWindow::on_pushButton_3_clicked()
{
    UpperSensor upperSensor;
    upperSensor.setModal(true);
    upperSensor.exec();
}

void MainWindow::on_pushButton_open_clicked()
{
    port.setQueryMode(QextSerialPort::EventDriven);
    port.setPortName("/dev/" + ui->comboBox_Interface->currentText());
    port.setBaudRate(BAUD115200);
    port.setFlowControl(FLOW_OFF);
    port.setParity(PAR_NONE);
    port.setDataBits(DATA_8);
    port.setStopBits(STOP_1);
    port.open(QIODevice::ReadWrite);

    if (!port.isOpen())
    {
        error.setText("Unable to open port!");
        error.show();
        return;
    }

    QObject::connect(&port, SIGNAL(readyRead()), this, SLOT(receive()));


    ui->pushButton_close->setEnabled(true);
    ui->pushButton_open->setEnabled(false);
    ui->comboBox_Interface->setEnabled(false);
}

void MainWindow::on_pushButton_close_clicked()
{
    if (port.isOpen())port.close();
    ui->pushButton_close->setEnabled(false);
    ui->pushButton_open->setEnabled(true);
    ui->comboBox_Interface->setEnabled(true);
}

void MainWindow::receive()
{

    static QString str;
        char ch;
        while (port.getChar(&ch))
        {
           str.append(ch);
            if (ch == '\n')     // End of line, start decoding
            {
                str.remove("\n", Qt::CaseSensitive);
                ui->textEdit->append(str);
            }
        }

    int temp_index, battery_index, status_index;

if (str.contains("RightSensor "))
{
                    QStringList list = str.split(QRegExp("\\s"));
                    qDebug() << "Str value: " << str;

                    if(!list.isEmpty())
                    {
                        qDebug() << "List size " << list.size();


                        // Temperature
                        double value;
                        for(int i = 0; i < list.size();i++)
                        {
                            if(list.at(i) == "RightSensor")
                            {
                                temp_index = i + 2;
                            }
                        }
                        value = list.at(temp_index).toDouble();
                        //adjust to Degrees
                        value = value / 1000;
                        printf("%f\n",value);
                        qDebug() << "Temperature var value " << QString::number(value);
                        ui->lcdNumber->display(value);
                        // Battery
                        double value_b;
                        for(int i = 0; i < list.size();i++)
                        {
                            if(list.at(i) == "RightSensor")
                            {
                                battery_index = i +1;
                            }
                        }
                        qDebug() << "Str value: " << str;
                        value_b = list.at(battery_index).toDouble();
                        //adjust to Degrees
                        //value_b = value_b / 1000;
                        printf("%f\n",value_b);
                        ui->progressBar->setMaximum(3400);
                        qDebug() << "Batery var value " << QString::number(value_b);
                        ui->progressBar->setValue((int)value_b);


                        //security
                        for(int i = 0; i < list.size();i++)
                        {
                            if(list.at(i) == "RightSensor")
                            {
                                status_index = i + 3;
                            }
                        }
                        //qDebug() << "Str value: " << str;
                        static QString Status;
                        if (list.at(status_index).toInt()==1)
                        {
                            Status = "DANGER";
                            ui->textEdit_2->setPlainText(Status);
                            ui->textEdit_2->setTextColor(QColor(255, 0, 0));
                        }
                        else if (list.at(status_index).toInt()== 0 )
                        {
                            Status = "SECURE";
                           ui->textEdit_2->setPlainText(Status);
                           ui->textEdit_2->setTextColor(QColor(0,200,100));
                        }

                    }

}

if (str.contains("LeftSensor "))
{
                    QStringList list = str.split(QRegExp("\\s"));
                    qDebug() << "Str value: " << str;

                    if(!list.isEmpty())
                    {
                        qDebug() << "List size " << list.size();
                        // Temperature
                        double value;for(int i = 0; i < list.size();i++)
                        {
                            if(list.at(i) == "LeftSensor")
                            {
                                temp_index = i + 2;
                            }
                        }
                        value = list.at(temp_index).toDouble();
                        //adjust to Degrees
                        value = value / 1000;
                        printf("%f\n",value);
                        qDebug() << "Temperature var value " << QString::number(value);
                        ui->lcdNumber_2->display(value);
                        // Battery
                        double value_b;
                        for(int i = 0; i < list.size();i++)
                        {
                            if(list.at(i) == "LeftSensor")
                            {
                                battery_index = i + 1;
                            }
                        }
                        qDebug() << "Str value: " << str;
                        value_b = list.at(battery_index).toDouble();
                        //adjust to Degrees
                        value_b = value_b / 1000;
                        printf("%f\n",value_b);
                        ui->progressBar_2->setMaximum(3.3);
                        qDebug() << "Batery var value " << QString::number(value_b);
                        ui->progressBar_2->setValue((int)value_b);

                        //security
                        for(int i = 0; i < list.size();i++)
                        {
                            if(list.at(i) == "LeftSensor")
                            {
                                status_index = i + 3;
                            }
                        }
                        //qDebug() << "Str value: " << str;
                        static QString Status;
                        if (list.at(status_index).toInt()==1)
                        {
                            Status = "DANGER";
                            ui->textEdit_3->setPlainText(Status);

                            ui->textEdit_3->setTextColor(QColor(255, 0, 0));
                        }
                        else if (list.at(status_index).toInt()== 0 )
                        {
                            Status = "SECURE";
                            ui->textEdit_3->setPlainText(Status);
                            ui->textEdit_3->setTextColor(QColor(0,200,100));

                        }


                    }
}

if (str.contains("FrontSensor "))
{
                    QStringList list = str.split(QRegExp("\\s"));
                    qDebug() << "Str value: " << str;

                    if(!list.isEmpty())
                    {
                        qDebug() << "List size " << list.size();
                        // Temperature
                        double value;
                        for(int i = 0; i < list.size();i++)
                        {
                            if(list.at(i) == "FrontSensor")
                            {
                                temp_index = i +2 ;
                            }
                        }
                        value = list.at(temp_index).toDouble();//adjust to Degrees
                        value = value / 1000;
                        printf("%f\n",value);
                        qDebug() << "Temperature var value " << QString::number(value);
                        ui->lcdNumber_3->display(value);
                        // Battery
                        double value_b;for(int i = 0; i < list.size();i++)
                        {
                            if(list.at(i) == "FrontSensor")
                            {
                                battery_index = i + 1;
                            }
                        }
                        qDebug() << "Str value: " << str;
                        value_b = list.at(battery_index).toDouble();
                        //adjust to Degrees
                        value_b = value_b / 1000;
                        printf("%f\n",value_b);
                        ui->progressBar_3->setMaximum(3.3);
                        qDebug() << "Batery var value " << QString::number(value_b);
                        ui->progressBar_3->setValue((int)value_b);

                    }

                    //security
                    for(int i = 0; i < list.size();i++)
                    {
                        if(list.at(i) == "FrontSensor")
                        {
                            status_index = i + 3;
                        }
                    }
                    //qDebug() << "Str value: " << str;
                    static QString Status;
                    if (list.at(status_index).toInt()==1)
                    {
                        Status = "DANGER";
                        ui->textEdit_3->setPlainText(Status);
                        ui->textEdit_3->setTextColor(QColor(0,0,255));

                    }
                    else if (list.at(status_index).toInt()== 0 )
                    {
                        Status = "SECURE";
                        ui->textEdit_3->setPlainText(Status);
                        ui->textEdit_3->setTextColor(QColor(0,200,100));
                    }

}
}
//End of Window receive
