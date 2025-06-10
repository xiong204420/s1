#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "HCNetSDK.h"
// #include <QNetworkProxy>

#define PUB_TOPIC "car/control"   // 发布主题
#define SUB_TOPIC "car/pm25"   // 订阅主题
#define MAX_SPEED 180
#define MIN_SPEED 0

using namespace Qt::StringLiterals;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);

    QSettings ini(QCoreApplication::applicationDirPath() + "/config.ini", QSettings::IniFormat, this);
    ui->edit_mqtt_host->setText(ini.value("mqtt/host", "127.0.0.1").toString());
    ui->edit_mqtt_post->setText(ini.value("mqtt/post", "1883").toString());
    ui->edit_mqtt_user->setText(ini.value("mqtt/user", "user").toString());
    ui->edit_mqtt_password->setText(ini.value("mqtt/password", "").toString());

    ui->edit_dvr_host->setText(ini.value("dvr/host", "127.0.0.1").toString());
    ui->edit_dvr_post->setText(ini.value("dvr/post", "8000").toString());
    ui->edit_dvr_user->setText(ini.value("dvr/user", "admin").toString());
    ui->edit_dvr_password->setText(ini.value("dvr/password", "").toString());

    ui->edit_title->setText(ini.value("sys/title", "").toString());
    ui->label_logo->setText(ui->edit_title->text());
    ui->spinbox_max_direction->setValue(ini.value("sys/maxdirection", 180).toInt());
    ui->spinbox_min_direction->setValue(ini.value("sys/mindirection", 0).toInt());
    ui->spinbox_step_direction->setValue(ini.value("sys/stepdirection", 5).toInt());

    max_direction = ui->spinbox_max_direction->value();
    min_direction = ui->spinbox_min_direction->value();
    step_direction = ui->spinbox_step_direction->value();

    m_client = new QMqttClient(this);
    connect(m_client, &QMqttClient::stateChanged, this, [ = ]() {
        QString state_s;
        switch(m_client->state()) {
        case QMqttClient::Disconnected:
            state_s = "连接断开。";
            ui->btn_mqttt_connect->setText("连接MQTT");
            break;
        case QMqttClient::Connecting:
            state_s = "连接中...";
            break;
        case QMqttClient::Connected:
            state_s = "连接成功！";
            ui->btn_mqttt_connect->setText("关闭MQTT");
            do_subscribe();
            break;
        }
        loger("MQTT状态：" + state_s);
    });
    connect(m_client, &QMqttClient::pingResponseReceived, this, [this]() {
        const QString content = QDateTime::currentDateTime().toString()
                                + " Ping Response."_L1;
        ui->edit_mqtt->append(content);
    });

    direction = 90;
    speed = 1;
    ui->label_direction->setText(QString::number(direction));
    ui->label_speed->setText(QString::number(speed));

    // QNetworkProxy proxy;
    // proxy.setType(QNetworkProxy::HttpProxy);
    // proxy.setHostName("127.0.0.1");
    // proxy.setPort(8008);
    // proxy.setUser("user");
    // proxy.setPassword("12345678");
    // QNetworkProxy::setApplicationProxy(proxy);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::loger(QString mes) {
    ui->edit_log->append(mes);
}

void MainWindow::do_publish() {
    direction = direction > max_direction ? max_direction : direction;
    direction = direction < min_direction ? min_direction : direction;
    speed = speed > MAX_SPEED ? MAX_SPEED : speed;
    speed = speed < MIN_SPEED ? MIN_SPEED : speed;
    QString d = "↑ ";
    d = direction < 90 ? "↖" : (direction > 90 ? "↗" : d);
    ui->label_direction->setText(d + QString::number(direction) + "º");
    // ui->label_speed->setText(QString::number(speed));
    QString mess = QString("%1,%2").arg(speed).arg(direction);
    if(m_client->publish(QMqttTopicName(PUB_TOPIC), mess.toUtf8(), 0, false) == -1) {
        loger(">发布信息失败！");
    }
    ui->edit_mqtt->append(QString(">> [%1]: %2").arg(PUB_TOPIC).arg(mess));
}

void MainWindow::do_subscribe() {
    subscription = m_client->subscribe(QMqttTopicFilter(SUB_TOPIC));
    if(!subscription) {
        QMessageBox::critical(this, u"Error"_s,
                              u"Could not subscribe. Is there a valid connection?"_s);
        return;
    }
    connect(subscription, &QMqttSubscription::messageReceived, this, [&](const QMqttMessage & msg) {
        ui->edit_mqtt->append(QString("<< [%1]: %2").arg(SUB_TOPIC).arg(msg.payload()));
        if(msg.topic().name() == SUB_TOPIC) {
            QString str = msg.payload();
            QStringList slist = str.split(",");
            ui->label_humidity->setText(slist[0] + "%");
            ui->label_temperature->setText(slist[1] + "℃");
            ui->label_pm->setText(slist[2] + "ppm");
        }
    });
}

bool MainWindow::event(QEvent *ev) {
    // 判断键盘按下事件
    if(ev->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);

        switch(keyEvent->key()) {
        case Qt::Key_A:
            on_btn_left_clicked();
            break;
        case Qt::Key_S:
            on_btn_down_clicked();
            break;
        case Qt::Key_W:
            on_btn_up_clicked();
            break;
        case Qt::Key_D:
            on_btn_right_clicked();
            break;
        }
    }

    // 其他事件交给父类，默认处理
    return QWidget::event(ev);
}


void MainWindow::on_btn_mqtt_save_clicked() {
    QSettings ini(QCoreApplication::applicationDirPath() + "/config.ini", QSettings::IniFormat, this);
    ini.setValue("mqtt/host", ui->edit_mqtt_host->text());
    ini.setValue("mqtt/post", ui->edit_mqtt_post->text());
    ini.setValue("mqtt/user", ui->edit_mqtt_user->text());
    ini.setValue("mqtt/password", ui->edit_mqtt_password->text());
    ui->statusbar->showMessage("MQTT 参数保存成功！", 5000);
}


void MainWindow::on_btn_dvr_save_clicked() {
    QSettings ini(QCoreApplication::applicationDirPath() + "/config.ini", QSettings::IniFormat, this);
    ini.setValue("dvr/host", ui->edit_dvr_host->text());
    ini.setValue("dvr/post", ui->edit_dvr_post->text());
    ini.setValue("dvr/user", ui->edit_dvr_user->text());
    ini.setValue("dvr/password", ui->edit_dvr_password->text());
    ui->statusbar->showMessage("摄像头参数保存成功！", 5000);
}


void MainWindow::on_btn_right_clicked() {
    direction += step_direction;
    do_publish();
    loger(QString("> 调整转向，方向：%1").arg(direction));
}


void MainWindow::on_btn_left_clicked() {
    direction -= step_direction;
    do_publish();
    loger(QString("< 调整转向，方向：%1").arg(direction));
}


void MainWindow::on_btn_up_clicked() {
    QString s;
    speed += 1;
    do_publish();
    switch(speed) {
    case 0:
        s = "↓ R";
        break;
    case 1:
        s = "- P";
        break;
    case 2:
        s = "↑ D1";
        break;
    case 3:
        s = "↑ D2";
        break;
    case 4:
        s = "↑ D3";
        break;
    }
    loger("速度调整：" + s);
    ui->label_speed->setText(s);
}


void MainWindow::on_btn_down_clicked() {
    QString s;
    speed -= 1;
    do_publish();

    switch(speed) {
    case 0:
        s = "↓ R";
        break;
    case 1:
        s = "- P";
        break;
    case 2:
        s = "↑ D1";
        break;
    case 3:
        s = "↑ D2";
        break;
    case 4:
        s = "↑ D3";
        break;
    }
    loger("速度调整：" + s);
    ui->label_speed->setText(s);
}


void MainWindow::on_btn_dvr_connect_clicked() {
    if(!dvr_connected) {
        // 1.初始化海康监控SDK
        bool isok = NET_DVR_Init();
        if(isok) {
            qDebug() << "SDK初始化成功!";
            loger("1.海康SDK初始化成功！连接摄像头：" + ui->edit_dvr_host->text() + ":" + ui->edit_dvr_post->text());
            //设置连接时间与重连时间
            // NET_DVR_SetConnectTime(3000, 3);
            // NET_DVR_SetReconnect(30000, true);
            // 2.设置连接相机的信息，用户注册设备
            NET_DVR_USER_LOGIN_INFO struLoginInfo; //包含相机参数的结构体
            NET_DVR_DEVICEINFO_V40 struDeviceInfoV40; //登录后的相机信息

            struLoginInfo.bUseAsynLogin = 0; //同步登录方式
            strcpy_s(struLoginInfo.sDeviceAddress, ui->edit_dvr_host->text().toLatin1()); //设备IP地址
            struLoginInfo.wPort = ui->edit_dvr_post->text().toInt(); //设备服务端口
            strcpy_s(struLoginInfo.sUserName, ui->edit_dvr_user->text().toLatin1()); //设备登录用户名
            strcpy_s(struLoginInfo.sPassword, ui->edit_dvr_password->text().toLatin1()); //设备登录密码

            lUserID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40); //设备登录

            if(lUserID < 0) {
                qDebug() << "登录失败: " << NET_DVR_GetLastError();
                loger("  ...摄像头连接失败，错误代码：" + QString::number(NET_DVR_GetLastError()));
                NET_DVR_Cleanup();//释放 SDK 资源
                return;
            } else {
                qDebug() << "登录成功 ";
                loger("  ...摄像头连接成功！");
                //            HWND hWnd = (HWND)ui->graphicsView->winId(); //获取窗口句柄
                HWND hWnd = (HWND)ui->graphicsView->winId(); //获取窗口句柄
                NET_DVR_PREVIEWINFO struPlayInfo;
                struPlayInfo.hPlayWnd = hWnd; //需要 SDK 解码时句柄设为有效值，仅取流不解码时可设为空
                struPlayInfo.lChannel = 1; //预览通道号
                struPlayInfo.dwStreamType = 0; //0-主码流，1-子码流，2-码流 3，3-码流 4，以此类推
                struPlayInfo.dwLinkMode = 0; //0- TCP 方式，1- UDP 方式，2- 多播方式，3- RTP 方式，4-RTP/RTSP，5-RSTP/HTTP
                struPlayInfo.bBlocked = 1; //0- 非阻塞取流，1- 阻塞取流
                lRealPlayHandle = NET_DVR_RealPlay_V40(lUserID, &struPlayInfo, nullptr, nullptr);

                if(lRealPlayHandle < 0) {
                    qDebug() << "预览失败: " << NET_DVR_GetLastError();
                    loger("  ....图像预览失败，错误代码：" + QString::number(NET_DVR_GetLastError()));
                    NET_DVR_Logout(lUserID);
                    NET_DVR_Cleanup();
                    return;
                }
                ui->btn_dvr_connect->setText("关闭摄像头");
                dvr_connected = true;
            }
        } else {
            qDebug() << "SDK初始化失败: " << NET_DVR_GetLastError();
            loger(">海康SDK初始化失败，错误代码：" + QString::number(NET_DVR_GetLastError()));
        }
    } else {
        //关闭预览
        NET_DVR_StopRealPlay(lRealPlayHandle);
        //注销用户
        NET_DVR_Logout(lUserID);
        NET_DVR_Cleanup();
        loger(">断开与摄像头连接。");
        ui->btn_dvr_connect->setText("连接摄像头");
        dvr_connected = false;
    }
}


void MainWindow::on_btn_mqttt_connect_clicked() {
    if(m_client->state() == QMqttClient::Disconnected) {
        m_client->setHostname(ui->edit_mqtt_host->text());
        m_client->setPort(ui->edit_mqtt_post->text().toInt());
        m_client->setUsername(ui->edit_mqtt_user->text());
        m_client->setPassword(ui->edit_mqtt_password->text());
        m_client->connectToHost();
    } else {
        subscription->unsubscribe();
        m_client->disconnectFromHost();
    }
}


void MainWindow::on_btn_other_clicked() {
    QSettings ini(QCoreApplication::applicationDirPath() + "/config.ini", QSettings::IniFormat, this);
    ini.setValue("sys/title", ui->edit_title->text());
    ini.setValue("sys/maxdirection", ui->spinbox_max_direction->value());
    ini.setValue("sys/mindirection", ui->spinbox_min_direction->value());
    ini.setValue("sys/stepdirection", ui->spinbox_step_direction->value());

    max_direction = ui->spinbox_max_direction->value();
    min_direction = ui->spinbox_min_direction->value();
    step_direction = ui->spinbox_step_direction->value();

    ui->label_logo->setText(ui->edit_title->text());
    ui->statusbar->showMessage("标题参数保存成功！", 5000);
}

