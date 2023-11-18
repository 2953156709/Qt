#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QtNetwork>
#include <QDebug>
#include <QString>
#include <QtCharts>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDateTime>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTextCodec>
#include <QButtonGroup>
#include "receivethread.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE


class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    QVector<QVariant> analysisData(QString ReceiveData);//解析数据
    void sendToMysql(QVector<QVariant> ReceiveData);//写入数据库
    void deviceShow(QVector<QVariant> ReceiveData);//设备列表显示
    void chartViewInit();//统计图初始化
    void sqlInit();//数据库初始化

private slots:
    void receiverData(QString ReceiveData);//接收子线程数据
    void on_upCheckBt_clicked();
    void on_downCheckBt_clicked();
    void on_endCheckBt_clicked();
    void on_cancelClickedBt_clicked();
    void on_upDeviceList_itemSelectionChanged();
    void on_downDeviceList_itemSelectionChanged();
    void on_tempCheckBox_stateChanged(int arg1);
    void on_highCheckBox_stateChanged(int arg1);
    void on_clearBt_clicked();
    void on_saveBt_clicked();
    void upReceiveEditShow();//上行接收区显示
    void downReceiveEditShow();//下行接收区显示
    void upDeviceInformShow();//上行设备信息显示
    void downDeviceInformShow();//下行设备信息显示
    void upTempChartShow();//上行温度统计图显示
    void upHighChartShow();//上行高度统计图显示
    void downTempChartShow();//下行温度统计图显示
    void downHighChartShow();//下行高度统计图显示
    void upSearchDevice();//上行读取数据库数据
    void downSearchDevice();//下行读取数据库数据

    void on_normalBt_clicked();

    void on_abnormalBt_clicked();

    void on_lastBt_clicked();

    void on_nextBt_clicked();

    void on_exchangeBt_clicked();

signals:
    void upCheckStart();//上行巡检开始
    void downCheckStart();//下行巡检开始
    void checkEnd();//巡检结束

private:
    Ui::Widget *ui;
    ReceiveThread *receiveThread;
    QVector<QVariant> mysqlData;
    QStringList receiveDataMeida;
    QChart *tempChart;
    QChart *highChart;
    QLineSeries *tempSeries;
    QLineSeries *highSeries;
    QValueAxis *axisTempX;
    QValueAxis *axisTempY;
    QValueAxis *axisHighX;
    QValueAxis *axisHighY;
    QSqlDatabase dbServer;
    QSqlDatabase db;
    QSqlQuery createDbQuery;
    QSqlQuery query;
    QDateTime currentTime;
    QString upTableName;
    QString downTableName;
    QString nowTime;
    int ID;
    int workDay;
    QVector<double> temp;
    QVector<double> high;
    QButtonGroup *radioGroup;



};
#endif // WIDGET_H
