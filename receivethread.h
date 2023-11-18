#ifndef RECEIVETHREAD_H
#define RECEIVETHREAD_H

#include <QObject>
#include <QThread>
#include <QUdpSocket>
#include <QQueue>
#include <QDataStream>
#include <QTimer>
#include <QDebug>

class ReceiveThread : public QThread
{
    Q_OBJECT
public:
    ReceiveThread();
    ~ReceiveThread();
    void run();
signals:
    void sendToMain(QString receiveData);//向主线程发送数据
public slots:
    void UpCheckStart(); //上行巡检定时器打开
    void DownCheckStart();//下行巡检定时器打开
    void receiver();//接收检测端数据
    void upStart();//发送上行设备激活信号
    void downStart();//发送下行设备激活信号
    void checkEnd();//上行下行定时器关闭
private:
    QUdpSocket *socket;
    quint16 localPort;
    QByteArray activateCmd;
    QHostAddress groupAddress;
    QByteArray receiveDataByte;
    QString receiveData;
    QTimer *upTime;
    QTimer *downTime;
    int upClickCnt = 0;
    int downClickCnt = 0;



};

#endif // RECEIVETHREAD_H
