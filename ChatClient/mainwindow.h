#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "chatclient.h"
#include <QString>
#include <QListWidget>

//新
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDir>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_loginButton_clicked();

    void on_sayButton_clicked();

    void on_layoutButton_clicked();

    void connectedToServer();
    void messageReceived(const QString &sender, const QString &text);
    void jsonReceived(const QJsonObject &docObj);
    void userJoined(const QString &user);
    void userLeft(const QString &user);
    void userListReceived(const QStringList &list);
    //新增
    void on_userlistWidget_itemDoubleClicked(QListWidgetItem *item);

    void on_exitPrivateButton_clicked();

private:
    Ui::MainWindow *ui;
    chatClient *m_chatclient;
    //新增
    QString m_myUsername;
    QString m_privateTarget;
    void exitPrivateMode();
    //数据库新增
    QSqlDatabase m_db;
    void initDatabase();
    void saveMessage(const QString &type, const QString &sender,
                     const QString &receiver, const QString &content);
    void saveUserLogin(const QString &nickname);
    void saveUserLogout(const QString &nickname);
    void loadHistory();
    void loadUserInfo();
};
#endif // MAINWINDOW_H
