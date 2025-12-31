#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHostAddress>
#include <QJsonValue>
#include <QJsonObject>
#include <QMessageBox>  // 添加这个头文件
#include <QJsonArray>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentWidget(ui->loginPage);
    m_chatclient = new chatClient(this);
    connect(m_chatclient, &chatClient::connected, this, &MainWindow::connectedToServer);
    // connect(m_chatclient,&chatClient::messageReceived,this,&MainWindow::messageReceived);
    connect(m_chatclient, &chatClient::jsonReceived, this, &MainWindow::jsonReceived);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_loginButton_clicked()
{
    if (ui->usernameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "登录失败", "用户名不能为空");
        return;
    }
    m_chatclient->connectToServer(QHostAddress(ui->serverEdit->text()), 1967);
}


//void MainWindow::on_sayButton_clicked()
//{

//    QString message = ui->saylineEdit->text();

//    if (!message.isEmpty()) {

//        m_chatclient->sendMessage(message);

//        ui->saylineEdit->clear();

//    }
//}

//修改
void MainWindow::on_sayButton_clicked()
{
    QString message = ui->saylineEdit->text().trimmed();
    if (message.isEmpty()) return;

    if (!m_privateTarget.isEmpty()) {
        // 发送私聊
        QJsonObject msg;
        msg["type"] = "private";
        msg["target"] = m_privateTarget;
        msg["text"] = message;

        m_chatclient->sendJson(msg); // ✅ 使用新方法

        // 本地回显（私聊）
        ui->roomtextEdit->append(QString("[私聊 → %1] %2").arg(m_privateTarget, message));
    } else {
        // 公共聊天
        QJsonObject msg;
        msg["type"] = "message";
        msg["text"] = message;

        m_chatclient->sendJson(msg); // ✅ 统一使用 sendJson

        // 可选：本地回显（公共）
        // QString selfName = /* 你的用户名 */;
        // ui->roomtextEdit->append(QString("[%1] %2").arg(selfName, message));
    }

    ui->saylineEdit->clear();
}

void MainWindow::on_layoutButton_clicked()

{
    m_privateTarget.clear();//新增
    m_chatclient->disconnectFromHost();
    ui->stackedWidget->setCurrentWidget(ui->loginPage);
    //新增
    ui->sayButton->setText("发送"); // 👈 恢复按钮文本
    ui ->saylineEdit->setPlaceholderText("");
    //结束
    for (auto aItem : ui->userlistWidget ->findItems(ui->usernameEdit->text(), Qt::MatchExactly)) {
        qDebug("remove");
        ui->userlistWidget->removeItemWidget(aItem);
        delete aItem;
    }
}

void MainWindow::connectedToServer()
{
    // 注意：这里只发送登录信息，不立即切换页面
    // 等待服务器返回成功后再切换
//    m_chatclient->sendMessage(ui->usernameEdit->text(), "login");
    //修改
    QString username = ui->usernameEdit->text().trimmed();
    QJsonObject loginMsg;
    loginMsg["type"] = "login";
    loginMsg["text"] = username;
    m_chatclient->sendJson(loginMsg);
}

void MainWindow::messageReceived(const QString &sender, const QString &text)
{
    ui->roomtextEdit->append(QString("%1 : %2").arg(sender).arg(text));
}

void MainWindow::jsonReceived(const QJsonObject &docObj)
{
    const QJsonValue typeVal = docObj.value("type");
    if (typeVal.isNull() || !typeVal.isString())
        return;

    // 添加处理登录错误
    if (typeVal.toString().compare("loginError", Qt::CaseInsensitive) == 0) {
        const QJsonValue textVal = docObj.value("text");
        if (textVal.isNull() || !textVal.isString())
            return;

        const QString errorMsg = textVal.toString();
        // 在客户端控制台输出错误信息
        qDebug() << "登录失败：" << errorMsg;

        // 显示错误消息给用户
        QMessageBox::warning(this, "登录失败", errorMsg);

        // 断开连接，让用户重新输入
        m_chatclient->disconnectFromHost();
        ui->stackedWidget->setCurrentWidget(ui->loginPage);
        return;
    }

    if (typeVal.toString().compare("message", Qt::CaseInsensitive) == 0) {
        const QJsonValue textVal = docObj.value("text");
        const QJsonValue senderVal = docObj.value("sender");
        if (textVal.isNull() || !textVal.isString())
            return;

        if (senderVal.isNull() || !senderVal.isString())
            return;

        const QString text = textVal.toString().trimmed();
        if (text.isEmpty())
            return;
        const QString sender = senderVal.toString().trimmed();
        if (text.isEmpty())
            return;

        messageReceived(sender, text);

    }     //新增
    else if (typeVal.toString().compare("private", Qt::CaseInsensitive) == 0) {
        const QJsonValue senderVal = docObj.value("sender");
        const QJsonValue textVal = docObj.value("text");
         if(   senderVal.isNull() || !senderVal.isString() ||
                textVal.isNull() || !textVal.isString()) {
            return;
        }
        QString sender = senderVal.toString().trimmed();
        QString text = textVal.toString().trimmed();
        // 显示私聊消息（带标识）
        ui->roomtextEdit->append(QString("[私聊 ← %1] %2").arg(sender, text));
    }
    //结束
    else if (typeVal.toString().compare("newuser", Qt::CaseInsensitive) == 0) {
        const QJsonValue usernameVal = docObj.value("username");
        if (usernameVal.isNull() || !usernameVal.isString())
            return;
        userJoined(usernameVal.toString());
    } else if (typeVal.toString().compare("userdisconnected", Qt::CaseInsensitive) == 0) {
        const QJsonValue usernameVal = docObj.value("username");
        if (usernameVal.isNull() || !usernameVal.isString())
            return;
        userLeft(usernameVal.toString());
    } else if (typeVal.toString().compare("userlist", Qt::CaseInsensitive) == 0) {
        // 收到用户列表，表示登录成功，切换页面
        if (ui->stackedWidget->currentWidget() != ui->chatPage) {
            ui->stackedWidget->setCurrentWidget(ui->chatPage);
        }

        const QJsonValue userlistVal = docObj.value("userlist");
        if (userlistVal.isNull() || !userlistVal.isArray())
            return;

        qDebug() << userlistVal.toVariant().toStringList();
        userListReceived(userlistVal.toVariant().toStringList());
    }
    //新增一段
    else if (typeVal.toString().compare("userlist", Qt::CaseInsensitive) == 0) {
        ui->stackedWidget->setCurrentWidget(ui->chatPage);
        m_myUsername = ui->usernameEdit->text().trimmed(); // ✅ 保存

        const QJsonValue userlistVal = docObj.value("userlist");
        if (userlistVal.isArray()) {
            QStringList list;
            for (const QJsonValue &v : userlistVal.toArray()) {
                list << v.toString();
            }
            userListReceived(list);
        }
    }
}

void MainWindow::userJoined(const QString &user)
{
    ui->userlistWidget->addItem(user);
}

void MainWindow::userLeft(const QString &user)
{
    for (auto aItem : ui->userlistWidget ->findItems(user, Qt::MatchExactly)) {
        qDebug("remove");
        ui->userlistWidget->removeItemWidget(aItem);
        delete aItem;
    }
}

void MainWindow::userListReceived(const QStringList &list)
{
    ui->userlistWidget->clear();
    ui->userlistWidget->addItems(list);
}
//新增
void MainWindow::on_userlistWidget_itemDoubleClicked(QListWidgetItem *item)
{
    QString username = item->text();
    // 去掉可能的“*”标记（自己）
    if (username.endsWith('*')) {
        username = username.left(username.length() - 1);
    }

    if (username == m_myUsername) {
        QMessageBox::information(this, "提示", "不能和自己私聊");
        return;
    }

    m_privateTarget = username;
    ui->saylineEdit->setPlaceholderText(QString("私聊 → %1").arg(m_privateTarget));
    ui->sayButton->setText("发送私聊");
}
