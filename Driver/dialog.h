#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <wiringPiI2C.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QTimer>

#define Device_Address 0x68	/*Device Address/Identifier for MPU6050*/

#define PWR_MGMT_1   0x6B
#define SMPLRT_DIV   0x19
#define CONFIG       0x1A
#define GYRO_CONFIG  0x1B
#define INT_ENABLE   0x38
#define ACCEL_XOUT_H 0x3B
#define ACCEL_YOUT_H 0x3D
#define ACCEL_ZOUT_H 0x3F
#define GYRO_XOUT_H  0x43
#define GYRO_YOUT_H  0x45
#define GYRO_ZOUT_H  0x47

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE

class Rpi{
public:
    Rpi();

    float getAx();
    float getAy();
    float getAz();

    float getGx();
    float getGy();
    float getGz();

    ~Rpi();
private:
    int fd;
    float Ax, Ay, Az;
    float Gx, Gy, Gz;

    void  MPU6050_Init();
    short read_raw_data(int addr);
};

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);
    ~Dialog();

private:
    Ui::Dialog *ui;
    Rpi mpu6050;

    float curr_time;

    QTimer *timer1;
    QtCharts::QLineSeries *Axseries;// = new QLineSeries();
    QtCharts::QLineSeries *Ayseries;// = new QLineSeries();
    QtCharts::QLineSeries *Azseries;// = new QLineSeries();
    QtCharts::QLineSeries *Gxseries;// = new QLineSeries();
    QtCharts::QLineSeries *Gyseries;// = new QLineSeries();
    QtCharts::QLineSeries *Gzseries;// = new QLineSeries();

    QtCharts::QChart *Axchart;// = new QChart();
    QtCharts::QChart *Aychart;// = new QChart();
    QtCharts::QChart *Azchart;// = new QChart();
    QtCharts::QChart *Gxchart;// = new QChart();
    QtCharts::QChart *Gychart;// = new QChart();
    QtCharts::QChart *Gzchart;// = new QChart();

public slots :
    void mpu6050_read();
};

#endif // DIALOG_H
