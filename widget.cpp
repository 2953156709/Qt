#include "widget.h"
#include "ui_widget.h".h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    chartViewInit();
    receiveThread = new ReceiveThread();
    receiveThread->start();
    currentTime = QDateTime::currentDateTime();
    nowTime = currentTime.toString("MM.dd");
    upTableName = "UP"+currentTime.toString("yyyy_MM_dd");
    downTableName = "DOWN"+currentTime.toString("yyyy_MM_dd");
    radioGroup = new QButtonGroup(this);
    radioGroup->addButton(ui->normalBt);
    radioGroup->addButton(ui->abnormalBt);
    sqlInit();
    connect(this,&Widget::upCheckStart,receiveThread,&ReceiveThread::UpCheckStart);
    connect(this,&Widget::downCheckStart,receiveThread,&ReceiveThread::DownCheckStart);
    connect(this,&Widget::checkEnd,receiveThread,&ReceiveThread::checkEnd);
    connect(receiveThread,&ReceiveThread::sendToMain,this,&Widget::receiverData);
}

Widget::~Widget()
{
    dbServer.close();
    db.close();
    delete ui;
}
//解析数据
QVector<QVariant> Widget::analysisData(QString ReceiveData)
{
    QVector<QVariant> receiveData;
    receiveDataMeida = ReceiveData.split('=');
    if(receiveDataMeida[0]=="#R")
    {
        receiveData.push_back('R');
        return receiveData;
    }
    else
    {
        receiveData.push_back('C');
        for(int i=1;i<receiveDataMeida.size();i++)
        {
            if(i<=3)
                receiveData.push_back(receiveDataMeida[i].toInt());
            else
                receiveData.push_back(receiveDataMeida[i].toDouble());
        }
        return receiveData;
    }
}

//接收子线程数据
void Widget::receiverData(QString ReceiveData)
{
    QVector<QVariant> receiveData = analysisData(ReceiveData);
    if(receiveData[0]=='C')
    {
        sendToMysql(receiveData);
        deviceShow(receiveData);
    }
}

//写入数据库
void Widget::sendToMysql(QVector<QVariant> ReceiveData)
{
    int cnt = ReceiveData.size();
    QString cname = "route,ID,workDay";
    QString cvalue = "?,?,?";
    QString update = "route=?,ID=?,workDay=?";
    for(int i=4;i<cnt;i++)
    {
        if(!(i%2))
        {
            cname += QString(",temp_%1").arg((i-4)/2);
            update += QString(",temp_%1=?").arg((i-4)/2);
        }
        else
        {
            cname += QString(",high_%1").arg((i-4)/2);
            update += QString(",high_%1=?").arg((i-4)/2);
        }
        cvalue += ",?";
    }
    if(ReceiveData[1]==0)
    {
        query.exec(QString("SELECT * FROM %1 WHERE ID=%2").arg(upTableName).arg(ReceiveData[2].toInt()));
        QString insertToSql = QString("INSERT INTO %1 (%2) VALUES (%3) ").arg(upTableName).arg(cname).arg(cvalue);
        QString updateToSql = QString("UPDATE %1 SET %2 WHERE ID=%3").arg(upTableName).arg(update).arg(ReceiveData[2].toInt());
        if(!query.size())
            query.prepare(insertToSql);
        else
            query.prepare(updateToSql);
        for(int i=1;i<cnt;i++)
        {

            query.addBindValue(ReceiveData[i]);
        }
        query.exec();
        query.lastError().text();
    }

    else
    {
        query.exec(QString("SELECT * FROM %1 WHERE ID=%2").arg(downTableName).arg(ReceiveData[2].toInt()));
        QString insertToSql = QString("INSERT INTO %1 (%2) VALUES (%3)").arg(downTableName).arg(cname).arg(cvalue);
        QString updateToSql = QString("UPDATE %1 SET %2 WHERE ID=%3").arg(downTableName).arg(update).arg(ReceiveData[2].toInt());
        if(!query.size())
            query.prepare(insertToSql);
        else
            query.prepare(updateToSql);
        for(int i=1;i<cnt;i++)
        {
            query.addBindValue(ReceiveData[i]);
        }
        query.exec();
    }
}
//设备列表显示
void Widget::deviceShow(QVector<QVariant> ReceiveData)
{
    if(ReceiveData[1]==0&&ui->upDeviceList->findItems(QString("上行设备%1").arg(ReceiveData[2].toString()),Qt::MatchExactly).isEmpty())
        ui->upDeviceList->addItem(QString("上行设备%1").arg(ReceiveData[2].toString()));
    else if(ReceiveData[1]==1&&ui->downDeviceList->findItems(QString("下行设备%1").arg(ReceiveData[2].toString()),Qt::MatchExactly).isEmpty())
        ui->downDeviceList->addItem(QString("下行设备%1").arg(ReceiveData[2].toString()));
}

//统计图初始化
void Widget::chartViewInit()
{
        tempChart = new QChart();
        highChart = new QChart();
        //隐藏图例
        tempChart->legend()->hide();
        highChart->legend()->hide();
        //设置折线图名称
        tempChart->setTitle("温度折线图");
        highChart->setTitle("高度折线图");
        //chartView与chart关联
        ui->tempChartView->setChart(tempChart);
        ui->highChartView->setChart(highChart);
        //每次改变数据点时刷新
        ui->tempChartView->setViewportUpdateMode(QChartView::FullViewportUpdate);
        ui->highChartView->setViewportUpdateMode(QChartView::FullViewportUpdate);
        //线型
        tempSeries = new QLineSeries;
        highSeries = new QLineSeries;
        //设置数据点标签
        QFont *font = new QFont();
        font->setPointSize(12);
        tempSeries->setPointLabelsClipping(false);
        highSeries->setPointLabelsClipping(false);
        tempSeries->setPointLabelsFormat("@yPoint");
        tempSeries->setPointLabelsVisible(true);
        tempSeries->setPointLabelsFont(*font);
        highSeries->setPointLabelsFormat("@yPoint");
        highSeries->setPointLabelsVisible(true);
        highSeries->setPointLabelsFont(*font);
        //设置点可见
        tempSeries->setPointsVisible(true);
        highSeries->setPointsVisible(true);
        //添加折线
        tempChart->addSeries(tempSeries);;
        highChart->addSeries(highSeries);
        //设置x,y轴
        axisTempX = new QValueAxis;
        axisTempY = new QValueAxis;
        axisHighX = new QValueAxis;
        axisHighY = new QValueAxis;
//        axisTempX->setTitleText(QString("温度/")+QChar(0x2103));
//        axisTempY->setTitleText("时间/h");
//        axisHighX->setTitleText(QString("温度/")+QChar(0x2103));
//        axisHighY->setTitleText("时间/h");
        //设置范围
        axisTempX->setRange(0,23);
        axisTempX->setLabelFormat("%d");
        axisTempY->setRange(-10,40);
        axisHighX->setRange(0,23);
        axisHighX->setLabelFormat("%d");
        axisHighY->setRange(0,10);
        //设置倒序
        axisTempX->setReverse(true);
        axisHighX->setReverse(true);
        //设置刻度个数
        axisTempX->setTickCount(24);
        axisTempY->setTickCount(10);
        axisHighX->setTickCount(24);
        axisHighY->setTickCount(10);
        //------------------------------------------------
        ui->tempChartView->chart()->setAxisX(axisTempX,tempSeries);
        ui->tempChartView->chart()->setAxisY(axisTempY,tempSeries);
        ui->highChartView->chart()->setAxisX(axisHighX,highSeries);
        ui->highChartView->chart()->setAxisY(axisHighY,highSeries);
        // 动画：能使曲线绘制显示的更平滑，过渡效果更好看
        tempChart->setAnimationOptions(QChart::SeriesAnimations);
        highChart->setAnimationOptions(QChart::SeriesAnimations);
        // 设置渲染：抗锯齿，如果不设置那么曲线就显得不平滑
        ui->tempChartView->setRenderHint(QPainter::Antialiasing);
        ui->highChartView->setRenderHint(QPainter::Antialiasing);
}
//数据库初始化
void Widget::sqlInit()
{
    dbServer = QSqlDatabase::addDatabase("QMYSQL");
    dbServer.setHostName("localhost");
    dbServer.setUserName("root");
    dbServer.setPassword("588023");
    createDbQuery = QSqlQuery(dbServer);
    if (dbServer.open())
    {
        if(!createDbQuery.exec("CREATE DATABASE IF NOT EXISTS checkData"))
        {
            QMessageBox::warning(this,"创建数据库错误",dbServer.lastError().text());
            return;
        }
    }
    else
    {
        QMessageBox::warning(this, "打开数据库服务器错误", dbServer.lastError().text());
        return;
    }
    db = QSqlDatabase::addDatabase("QMYSQL","new_connection");
    db.setHostName("localhost");
    db.setUserName("root");
    db.setDatabaseName("checkData");
    db.setPassword("588023");
    query = QSqlQuery(db);
    if(!db.open())
    {
        QMessageBox::warning(this,"打开数据库错误",db.lastError().text());
        return;
    }
    query.exec(QString("DROP TABLE %1").arg(upTableName));
    query.exec(QString("DROP TABLE %1").arg(downTableName));
    QString upTableCreate = QString("CREATE TABLE IF NOT EXISTS `%1` (ind INT AUTO_INCREMENT PRIMARY KEY NOT NULL,route INT,ID INT,workDay INT").arg(upTableName);
    QString downTableCreate = QString("CREATE TABLE IF NOT EXISTS `%1` (ind INT AUTO_INCREMENT PRIMARY KEY NOT NULL,route INT,ID INT,workDay INT").arg(downTableName);
    for(int i=0;i<24;i++)
    {
        upTableCreate += QString(",temp_%1 DOUBLE,high_%2 DOUBLE").arg(i).arg(i);
        downTableCreate += QString(",temp_%1 DOUBLE,high_%2 DOUBLE").arg(i).arg(i);
    }
    upTableCreate += ") PARTITION BY RANGE(ind)(";
    upTableCreate += "PARTITION p0 VALUES LESS THAN (1000),";
    upTableCreate += "PARTITION p1 VALUES LESS THAN (2000),";
    upTableCreate += "PARTITION p2 VALUES LESS THAN (3000),";
    upTableCreate += "PARTITION p3 VALUES LESS THAN MAXVALUE)";
    downTableCreate += ") PARTITION BY RANGE(ind)(";
    downTableCreate += "PARTITION p0 VALUES LESS THAN (1000),";
    downTableCreate += "PARTITION p1 VALUES LESS THAN (2000),";
    downTableCreate += "PARTITION p2 VALUES LESS THAN (3000),";
    downTableCreate += "PARTITION p3 VALUES LESS THAN MAXVALUE)";
    //创建此次巡检数据表
    query.exec(upTableCreate);
    query.exec(downTableCreate);
    //创建汇总表
    query.exec("CREATE TABLE IF NOT EXISTS upTotal (ID INT PRIMARY KEY)");
    query.exec("CREATE TABLE IF NOT EXISTS downTotal (ID INT PRIMARY KEY)");
    for(int i=0;i<99;i++)
    {
        query.exec(QString("INSERT INTO upTotal (ID) VALUES (%1)").arg(1001+i));
        query.exec(QString("INSERT INTO downTotal (ID) VALUES (%1)").arg(1001+i));
    }
    //添加此次巡检汇总列
    query.exec(QString("ALTER TABLE upTotal ADD COLUMN `%1` VARCHAR(50)").arg(nowTime));
    query.exec(QString("ALTER TABLE downTotal ADD COLUMN `%1` VARCHAR(50)").arg(nowTime));
}



//上行接收区显示
void Widget::upReceiveEditShow()
{
    ui->receiveEdit->clear();
    for(int i=0;i<temp.size();i++)
    {
        ui->receiveEdit->appendPlainText(QString("ID:%1;工作天数:%2温度:%3;高度:%4---%5\n").arg(ID).arg(workDay).arg(temp[i]).arg(high[i]).arg(i));
    }
}
//下行接收区显示
void Widget::downReceiveEditShow()
{
    ui->receiveEdit->clear();
    for(int i=0;i<temp.size();i++)
    {
        ui->receiveEdit->appendPlainText(QString("ID:%1;工作天数:%2温度:%3;高度:%4---%5\n").arg(ID).arg(workDay).arg(temp[i]).arg(high[i]).arg(i));
    }
}
//上行设备信息显示
void Widget::upDeviceInformShow()
{
    ui->IDLabel->setText(QString::number(ID));
    ui->workDayLabel->setText(QString::number(workDay)+" 天");
    ui->tempLabel->setText(QString::number(temp[temp.size()-1])+' '+QChar(0x2103));
    ui->highLabel->setText(QString::number(high[temp.size()-1])+" m");
}
//下行设备信息显示
void Widget::downDeviceInformShow()
{
    ui->IDLabel->setText(QString::number(ID));
    ui->workDayLabel->setText(QString::number(workDay)+" 天");
    ui->tempLabel->setText(QString::number(temp[temp.size()-1])+' '+QChar(0x2103));
    ui->highLabel->setText(QString::number(high[temp.size()-1])+" m");
}
//上行温度统计图显示
void Widget::upTempChartShow()
{
    tempSeries->clear();
    for(int i=0;i<temp.size();i++)
    {
        tempSeries->append(i,temp[i]);
    }
}
//上行高度统计图显示
void Widget::upHighChartShow()
{
    highSeries->clear();
    for(int i=0;i<temp.size();i++)
    {
        highSeries->append(i,high[i]);
    }
}
//下行温度统计图显示
void Widget::downTempChartShow()
{
    tempSeries->clear();
    for(int i=0;i<temp.size();i++)
    {
        tempSeries->append(i,temp[i]);
    }
}
//下行高度统计图显示
void Widget::downHighChartShow()
{
    highSeries->clear();
    for(int i=0;i<temp.size();i++)
    {
        highSeries->append(i,high[i]);
    }
}

//上行读取数据库数据
void Widget::upSearchDevice()
{
    temp.clear();
    high.clear();
    QString partition;
    int index = ui->upDeviceList->currentIndex().row()+1;
    if(index<=1000)
        partition = "p0";
    else if(index<=2000)
        partition = "p1";
    else if(index<=3000)
        partition = "p2";
    else
        partition = "p3";
    QString haveData = QString("SELECT * FROM %1 PARTITION (%2) WHERE ind=%3").arg(upTableName).arg(partition).arg(index);
    query.clear();
    query.exec(haveData);
    if(query.next())
    {
        ID = query.value(2).toInt();
        workDay = query.value(3).toInt();
    }
    for(int i=0;i<48&&!query.value(i+4).isNull();i++)
    {
        if((i+4)%2==0)
            temp.push_back(query.value(i+4).toDouble());
        else
            high.push_back(query.value(i+4).toDouble());
    }
}

//下行读取数据库数据
void Widget::downSearchDevice()
{
    temp.clear();
    high.clear();
    QString partition;
    int index = ui->downDeviceList->currentIndex().row()+1;
    if(index<=1000)
        partition = "p0";
    else if(index<=2000)
        partition = "p1";
    else if(index<=3000)
        partition = "p2";
    else
        partition = "p3";
    QString haveData = QString("SELECT * FROM %1 PARTITION (%2) WHERE ind=%3").arg(downTableName).arg(partition).arg(index);
    query.clear();
    query.exec(haveData);
    if(query.next())
    {
        ID = query.value(2).toInt();
        workDay = query.value(3).toInt();
    }
    for(int i=0;i<48&&!query.value(i+4).isNull();i++)
    {
        if((i+4)%2==0)
            temp.push_back(query.value(i+4).toDouble());
        else
            high.push_back(query.value(i+4).toDouble());
    }

}

//上行巡检点击
void Widget::on_upCheckBt_clicked()
{
    upCheckStart();
    ui->upCheckBt->setEnabled(false);
    ui->endCheckBt->setEnabled(true);
}

//下行巡检点击
void Widget::on_downCheckBt_clicked()
{
    downCheckStart();
    ui->downCheckBt->setEnabled(false);
    ui->endCheckBt->setEnabled(true);
}

//结束巡检点击
void Widget::on_endCheckBt_clicked()
{
    ui->upCheckBt->setEnabled(true);
    ui->downCheckBt->setEnabled(true);
    ui->endCheckBt->setEnabled(false);
    checkEnd();
}

//取消标记点击
void Widget::on_cancelClickedBt_clicked()
{
    ui->upDeviceList->clearSelection();
    ui->downDeviceList->clearSelection();
    ui->receiveEdit->clear();
    ui->IDLabel->clear();
    ui->workDayLabel->clear();
    ui->tempLabel->clear();
    ui->highLabel->clear();
    tempSeries->clear();
    highSeries->clear();
    ui->normalBt->setCheckable(false);
    ui->abnormalBt->setCheckable(false);
    ui->normalBt->update();
    ui->abnormalBt->update();

}

//上行列表设备选中状态改变
void Widget::on_upDeviceList_itemSelectionChanged()
{
    if(!ui->upDeviceList->selectedItems().isEmpty())
    {
        ui->downDeviceList->clearSelection();
        upSearchDevice();
        upReceiveEditShow();
        upDeviceInformShow();
        if(ui->tempCheckBox->isChecked())
            upTempChartShow();
        if(ui->highCheckBox->isChecked())
            upHighChartShow();
        ui->normalBt->setCheckable(true);
        ui->abnormalBt->setCheckable(true);
        radioGroup->setExclusive(false);
        if(radioGroup->checkedButton())
            radioGroup->checkedButton()->setChecked(false);
        radioGroup->setExclusive(true);
        ui->normalBt->update();
        ui->abnormalBt->update();

    }
}

//下行列表设备选中状态改变
void Widget::on_downDeviceList_itemSelectionChanged()
{
    if(!ui->downDeviceList->selectedItems().isEmpty())
    {

        ui->upDeviceList->clearSelection();
        downSearchDevice();
        downReceiveEditShow();
        downDeviceInformShow();
        if(ui->tempCheckBox->isChecked())
            downTempChartShow();
        if(ui->highCheckBox->isChecked())
            downHighChartShow();
        ui->normalBt->setCheckable(true);
        ui->abnormalBt->setCheckable(true);
        radioGroup->setExclusive(false);
        if(radioGroup->checkedButton())
            radioGroup->checkedButton()->setChecked(false);
        radioGroup->setExclusive(true);
        ui->normalBt->update();
        ui->abnormalBt->update();
    }
}

//温度图显示状态改变
void Widget::on_tempCheckBox_stateChanged(int arg1)
{
    if(ui->tempCheckBox->isChecked())
    {
        if(!ui->upDeviceList->selectedItems().isEmpty())
                upTempChartShow();
        else if(!ui->downDeviceList->selectedItems().isEmpty())
                downTempChartShow();
    }
    else
        tempSeries->clear();
}

//高度图显示状态改变
void Widget::on_highCheckBox_stateChanged(int arg1)
{
    if(ui->highCheckBox->isChecked())
    {
        if(!ui->upDeviceList->selectedItems().isEmpty())
                upHighChartShow();
        else if(!ui->downDeviceList->selectedItems().isEmpty())
                downHighChartShow();
    }
    else
        highSeries->clear();
}

//清除此次巡检记录（接受到的信息）
void Widget::on_clearBt_clicked()
{
    emit checkEnd();
    ui->upDeviceList->clear();
    ui->downDeviceList->clear();
    ui->receiveEdit->clear();
    ui->IDLabel->clear();
    ui->workDayLabel->clear();
    ui->tempLabel->clear();
    ui->highLabel->clear();
    tempSeries->clear();
    highSeries->clear();
    ui->upCheckBt->setEnabled(true);
    ui->downCheckBt->setEnabled(true);
    ui->endCheckBt->setEnabled(false);
    ui->normalBt->setCheckable(false);
    ui->abnormalBt->setCheckable(false);
    ui->normalBt->update();
    ui->abnormalBt->update();
    query.exec(QString("TRUNCATE TABLE %1").arg(upTableName));
    query.exec(QString("TRUNCATE TABLE %1").arg(downTableName));
}

//保存汇总表
void Widget::on_saveBt_clicked()
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
//    QSqlQuery informQuery(db);
//    QSqlQuery tempQuery(db);
//    QSqlQuery highQuery(db);
    QString upFileName = currentTime.toString("yyyy-MM-dd hh-mm-ssUP");
    QString upFilePath = QString("/var/lib/mysql-files/")+upFileName+".csv";
    QString downFileName = currentTime.toString("yyyy-MM-dd hh-mm-ssDOWN");
    QString downFilePath = QString("/var/lib/mysql-files/")+upFileName+".csv";
    query.exec(QString("SELECT * INTO OUTFILE '%1' FIELDS TERMINATED BY ',' LINES TERMINATED BY '\n' FROM upTotal").arg(upFilePath));
    query.exec(QString("SELECT * INTO OUTFILE '%1' FIELDS TERMINATED BY ',' LINES TERMINATED BY '\n' FROM downTotal").arg(downFilePath));
//    QFile file(filePath);
//    if(file.open(QIODevice::WriteOnly|QIODevice::Text))
//    {
//        QTextStream stream(&file);
//        stream.setCodec("UTF-8");
//        informQuery.exec("SELECT * FROM upDeviceInform");
//        tempQuery.exec("SELECT * FROM upTemp");
//        highQuery.exec("SELECT * FROM upHigh");
//        stream<<"UP_DEVICE\n";
//        while(informQuery.next()&&tempQuery.next()&&highQuery.next())
//        {
//                stream<<"      "<<informQuery.value(0).toInt()<<". ";
//                stream<<informQuery.value(1).toString()<<"  ";
//                for(int i=0;i<24&&tempQuery.value(i+1)!=0;i++)
//                {
//                    stream<<tempQuery.value(i+1).toDouble()<<"/";
//                    stream<<highQuery.value(i+1).toDouble()<<"  ";
//                }
//                stream<<"\n";
//        }
//        informQuery.exec("SELECT * FROM downDeviceInform");
//        tempQuery.exec("SELECT * FROM downTemp");
//        highQuery.exec("SELECT * FROM downHigh");
//        stream<<"DOWN_DEVICE\n";
//        while(informQuery.next()&&tempQuery.next()&&highQuery.next())
//        {
//                stream<<"      "<<informQuery.value(0).toInt()<<". ";
//                stream<<informQuery.value(1).toString()<<"  ";
//                for(int i=0;i<24&&tempQuery.value(i+1)!=0;i++)
//                {
//                    stream<<tempQuery.value(i+1).toDouble()<<"/";
//                    stream<<highQuery.value(i+1).toDouble()<<"  ";
//                }
//                stream<<"\n";
//        }
//        file.close();
//        QMessageBox::information(this,"提示","保存成功！");
//    }
//    else
//        QMessageBox::information(this,"提示","保存失败！");
}

//设备正常状态选中触发
void Widget::on_normalBt_clicked()
{
    if(ui->normalBt->isChecked())
    {
        if(!ui->upDeviceList->selectedItems().isEmpty())
            query.exec(QString("UPDATE upTotal SET `%1`='正常' WHERE ID=%2").arg(nowTime).arg(ID));
        else if(!ui->downDeviceList->selectedItems().isEmpty())
            query.exec(QString("UPDATE downTotal SET `%1`='正常' WHERE ID=%2").arg(nowTime).arg(ID));
    }
}

//设备异常状态选中触发
void Widget::on_abnormalBt_clicked()
{
    if(ui->abnormalBt->isChecked())
    {
        if(!ui->upDeviceList->selectedItems().isEmpty())
            query.exec(QString("UPDATE upTotal SET `%1`='异常' WHERE ID=%2").arg(nowTime).arg(ID));
        else if(!ui->downDeviceList->selectedItems().isEmpty())
            query.exec(QString("UPDATE downTotal SET `%1`='异常' WHERE ID=%2").arg(nowTime).arg(ID));
    }
}

//上一个设备点击
void Widget::on_lastBt_clicked()
{
    if(!ui->upDeviceList->selectedItems().isEmpty()&&ui->upDeviceList->currentRow()>0)
        ui->upDeviceList->setCurrentRow(ui->upDeviceList->currentRow()-1);
    else if(!ui->downDeviceList->selectedItems().isEmpty()&&ui->downDeviceList->currentRow()>0)
        ui->downDeviceList->setCurrentRow(ui->downDeviceList->currentRow()-1);
}

//下一个设备点击
void Widget::on_nextBt_clicked()
{
    if(!ui->upDeviceList->selectedItems().isEmpty()&&ui->upDeviceList->currentRow()<ui->upDeviceList->count()-1)
        ui->upDeviceList->setCurrentRow(ui->upDeviceList->currentRow()+1);
    else if(!ui->downDeviceList->selectedItems().isEmpty()&&ui->downDeviceList->currentRow()<ui->downDeviceList->count()-1)
        ui->downDeviceList->setCurrentRow(ui->downDeviceList->currentRow()+1);
}

//切换列表点击
void Widget::on_exchangeBt_clicked()
{
    if(!ui->upDeviceList->selectedItems().isEmpty()&&ui->downDeviceList->count()>0)
        ui->downDeviceList->setCurrentRow(0);
    else if(ui->upDeviceList->count()>0)
        ui->upDeviceList->setCurrentRow(0);
}

