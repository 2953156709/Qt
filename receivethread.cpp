#include "receivethread.h"

ReceiveThread::ReceiveThread()
{
    activateCmd = "activate";
    localPort = 1040;
    groupAddress = QHostAddress("239.0.0.1");
    //设置定时器信息
    upTime = new QTimer;
    downTime = new QTimer;
    upTime->setInterval(500);
    connect(upTime,&QTimer::timeout,this,&ReceiveThread::upStart);
    downTime->setInterval(500);
    connect(downTime,&QTimer::timeout,this,&ReceiveThread::downStart);
}

ReceiveThread::~ReceiveThread()
{
    delete upTime;
    delete downTime;
    delete socket;
    requestInterruption();
    wait();
}

void ReceiveThread::run()
{
    socket = new QUdpSocket;
    socket->setSocketOption(QAbstractSocket::MulticastTtlOption,1);
    socket->bind(QHostAddress::AnyIPv4,localPort,QUdpSocket::ShareAddress);
    socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,4*1024*1024);
    connect(socket,&QUdpSocket::readyRead,this,&ReceiveThread::receiver,Qt::DirectConnection);
    exec();
}
//上行巡检定时器打开
void ReceiveThread::UpCheckStart()
{
    upTime->start();
}
//下行巡检定时器打开
void ReceiveThread::DownCheckStart()
{
    downTime->start();
}
//接收检测端数据
void ReceiveThread::receiver()
{
    qRegisterMetaType<QQueue<double>>("QQueue<double>");
    receiveDataByte.resize(socket->pendingDatagramSize());
    socket->readDatagram(receiveDataByte.data(),receiveDataByte.size());
    receiveData = QString::fromUtf8(receiveDataByte);
    sendToMain(receiveData);
}
//发送上行设备激活信号
void ReceiveThread::upStart()
{
    for(int i=0;i<100;i++)
        socket->writeDatagram(activateCmd,groupAddress,1101+i);
}
//发送下行设备激活信号
void ReceiveThread::downStart()
{
    for(int i=0;i<100;i++)
        socket->writeDatagram(activateCmd,groupAddress,1201+i);
}
//上行下行定时器关闭
void ReceiveThread::checkEnd()
{
    upTime->stop();
    downTime->stop();
}
