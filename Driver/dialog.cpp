#include "dialog.h"
#include "ui_dialog.h"

QT_CHARTS_USE_NAMESPACE

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
    , mpu6050()
{
    ui->setupUi(this);

    QTimer *timer1 = new QTimer(this);
    connect(timer1, SIGNAL(timeout()), this, SLOT(mpu6050_read()));
    timer1->start(200);

    curr_time = 0;

    QLineSeries *Axseries = new QLineSeries();
    QLineSeries *Ayseries = new QLineSeries();
    QLineSeries *Azseries = new QLineSeries();
    QLineSeries *Gxseries = new QLineSeries();
    QLineSeries *Gyseries = new QLineSeries();
    QLineSeries *Gzseries = new QLineSeries();

    QChart *Axchart = new QChart();
    QChart *Aychart = new QChart();
    QChart *Azchart = new QChart();
    QChart *Gxchart = new QChart();
    QChart *Gychart = new QChart();
    QChart *Gzchart = new QChart();

    QChartView *AxchartView = new QChartView(Axchart);
    QChartView *AychartView = new QChartView(Aychart);
    QChartView *AzchartView = new QChartView(Azchart);
    QChartView *GxchartView = new QChartView(Gxchart);
    QChartView *GychartView = new QChartView(Gychart);
    QChartView *GzchartView = new QChartView(Gzchart);

    AxchartView->setRenderHint(QPainter::Antialiasing);
    AychartView->setRenderHint(QPainter::Antialiasing);
    AzchartView->setRenderHint(QPainter::Antialiasing);
    GxchartView->setRenderHint(QPainter::Antialiasing);
    GychartView->setRenderHint(QPainter::Antialiasing);
    GzchartView->setRenderHint(QPainter::Antialiasing);

    Axchart->setTitle("Accelerometer X coordinate");
    Aychart->setTitle("Accelerometer Y coordinate");
    Azchart->setTitle("Accelerometer Z coordinate");
    Gxchart->setTitle("Gyro X coordinate");
    Gychart->setTitle("Gyro Y coordinate");
    Gzchart->setTitle("Gyro Z coordinate");

    Axchart->legend()->hide();
    Aychart->legend()->hide();
    Azchart->legend()->hide();
    Gxchart->legend()->hide();
    Gychart->legend()->hide();
    Gzchart->legend()->hide();


    Axchart->createDefaultAxes();
    Aychart->createDefaultAxes();
    Azchart->createDefaultAxes();
    Gxchart->createDefaultAxes();
    Gychart->createDefaultAxes();
    Gzchart->createDefaultAxes();

    Axchart->axisX()->setRange(0,3.3);
    Aychart->axisX()->setRange(0,3.3);
    Azchart->axisX()->setRange(0,3.3);
    Gxchart->axisY()->setRange(0,3.3);
    Gychart->axisY()->setRange(0,3.3);
    Gzchart->axisY()->setRange(0,3.3);

    ui->verticalLayout->addWidget(AxchartView);
}

void Dialog::mpu6050_read()
{
    curr_time += 0.2;
    Axseries->append(curr_time, mpu6050.getAx());
    Ayseries->append(curr_time, mpu6050.getAy());
    Azseries->append(curr_time, mpu6050.getAz());

    Gxseries->append(curr_time, mpu6050.getAx());
    Gyseries->append(curr_time, mpu6050.getGy());
    Gzseries->append(curr_time, mpu6050.getGz());

    Axchart->addSeries(Axseries);
    Aychart->addSeries(Ayseries);
    Azchart->addSeries(Azseries);

    Gxchart->addSeries(Gxseries);
    Gychart->addSeries(Gyseries);
    Gzchart->addSeries(Gzseries);

}

Rpi::Rpi()
{
    fd = wiringPiI2CSetup(Device_Address);
    MPU6050_Init();

    Ax=0, Ay=0, Az=0;
    Gx=0, Gy=0, Gz=0;
}

void Rpi::MPU6050_Init(){
    if(fd < 0)
    {
        printf("fd error"); /*Initializes I2C with device Address*/
    }
    wiringPiI2CWriteReg8 (fd, SMPLRT_DIV, 0x07);	/* Write to sample rate register        */
    wiringPiI2CWriteReg8 (fd, PWR_MGMT_1, 0x01);	/* Write to power management register   */
    wiringPiI2CWriteReg8 (fd, CONFIG, 0);		    /* Write to Configuration register      */
    wiringPiI2CWriteReg8 (fd, GYRO_CONFIG, 24);	    /* Write to Gyro Configuration register */
    wiringPiI2CWriteReg8 (fd, INT_ENABLE, 0x01);	/* Write to interrupt enable register   */
}

short Rpi::read_raw_data(int addr){
    short high_byte,low_byte,value;
    high_byte = wiringPiI2CReadReg8(fd, addr);
    low_byte = wiringPiI2CReadReg8(fd, addr+1);
    value = (high_byte << 8) | low_byte;
    return value;
}

float Rpi::getAx() {
    return read_raw_data(ACCEL_XOUT_H / 16384.0);}
float Rpi::getAy() {
    return read_raw_data(ACCEL_ZOUT_H / 16384.0);}
float Rpi::getAz() {
    return read_raw_data(ACCEL_YOUT_H / 16384.0);}

float Rpi::getGx() {
    return read_raw_data(GYRO_XOUT_H / 131);}
float Rpi::getGy() {
    return read_raw_data(GYRO_YOUT_H / 131);}
float Rpi::getGz() {
    return read_raw_data(GYRO_ZOUT_H / 131);}

Dialog::~Dialog()
{
    delete ui;
}

Rpi::~Rpi(){}
