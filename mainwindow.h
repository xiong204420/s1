#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QSettings>
#include <QEvent>
#include <QKeyEvent>
#include <QtMqtt/QMqttClient>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void on_btn_mqtt_save_clicked();

    void on_btn_dvr_save_clicked();

    void on_btn_right_clicked();

    void on_btn_left_clicked();

    void on_btn_up_clicked();

    void on_btn_down_clicked();

    void on_btn_dvr_connect_clicked();

    void on_btn_mqttt_connect_clicked();

    void on_btn_other_clicked();

private:
    void loger(QString mes);
    void do_publish();
    void do_subscribe();

protected:
    // 事件分发
    bool event(QEvent *ev);

private:
    Ui::MainWindow *ui;
    QMqttClient *m_client;
    QMqttSubscription *subscription;

    int lUserID;
    bool dvr_connected = false,
         mqtt_connected = false;
    long lRealPlayHandle;
    int direction = 60,
        speed = 1,
        max_direction = 180,
        min_direction = 0,
        step_direction = 5;
};
#endif // MAINWINDOW_H
