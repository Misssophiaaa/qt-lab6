#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHostAddress>
#include <QJsonValue>
#include <QJsonObject>
#include <QMessageBox>  // 添加这个头文件
#include <QJsonArray>
#include <QSqlError>
#include <QDateTime>


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
//新增
    ui->exitPrivateButton->setEnabled(false);
    initDatabase();
}

MainWindow::~MainWindow()
{
    //新增
    if (!m_myUsername.isEmpty()) {
        saveUserLogout(m_myUsername); // 👈 记录登出
    }
    //结束
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
     //数据库新增
     QString username = ui->usernameEdit->text().trimmed();



    m_myUsername =
username;



    QJsonObject loginObj;
    loginObj["type"] = "login";
     loginObj["username"] = username;
    m_chatclient->sendJson(loginObj);
     // 新增结束
    if (message.isEmpty()) return;

    if (!m_privateTarget.isEmpty()) {
        // 发送私聊
        QJsonObject msg;
        msg["type"] = "private";
        msg["target"] = m_privateTarget;
        msg["text"] = message;

        m_chatclient->sendJson(msg); //  使用新方法

        // 本地回显（私聊）
         ui->roomtextEdit->append(QString("[私聊 → %1] %2").arg(m_privateTarget, message));
    } else {
        // 公共聊天
        QJsonObject msg;
        msg["type"] = "message";
        msg["text"] = message;

        m_chatclient->sendJson(msg); // 统一使用 sendJson

         //数据库新增
         //保存自己发出的群聊消息
         saveMessage("group", m_myUsername, "", message);
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
//群聊信息
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
         saveMessage("group", sender, "", text);

    }     //新增
    else if (typeVal.toString().compare("private", Qt::CaseInsensitive) == 0) {
        const QJsonValue senderVal = docObj.value("sender");
        const QJsonValue textVal = docObj.value("text");
         if  (senderVal.isNull() || !senderVal.isString() ||
                textVal.isNull() || !textVal.isString()) {
            return;
        }
        QString sender = senderVal.toString().trimmed();
        QString text = textVal.toString().trimmed();
        // 显示私聊消息（带标识）
        ui->roomtextEdit->append(QString("[私聊 ← %1] %2").arg(sender, text));


       saveMessage("private", sender, m_myUsername, text);
    }
    //结束
    //新用户加入
    else if (typeVal.toString().compare("newuser", Qt::CaseInsensitive) == 0) {
        const QJsonValue usernameVal = docObj.value("username");
        if (usernameVal.isNull() || !usernameVal.isString())
            return;
        userJoined(usernameVal.toString());
    }
    //   用户离开
    else if (typeVal.toString().compare("userdisconnected", Qt::CaseInsensitive) == 0) {
        const QJsonValue usernameVal = docObj.value("username");
        if (usernameVal.isNull() || !usernameVal.isString())
            return;
        userLeft(usernameVal.toString());
    }
    //  用户列表，下面先注释
//    else if (typeVal.toString().compare("userlist", Qt::CaseInsensitive) == 0) {
//        // 收到用户列表，表示登录成功，切换页面
//        if (ui->stackedWidget->currentWidget() != ui->chatPage) {
//            ui->stackedWidget->setCurrentWidget(ui->chatPage);
//        }

//        const QJsonValue userlistVal = docObj.value("userlist");
//        if (userlistVal.isNull() || !userlistVal.isArray())
//            return;

//        qDebug() << userlistVal.toVariant().toStringList();
//        userListReceived(userlistVal.toVariant().toStringList());
//    }
    //新增一段
    else if (typeVal.toString().compare("userlist", Qt::CaseInsensitive) == 0) {
        ui->stackedWidget->setCurrentWidget(ui->chatPage);
        m_myUsername = ui->usernameEdit->text().trimmed(); //  保存

        //数据库新增：
        saveUserLogin(m_myUsername);//  记录用户登录到数据库
        loadHistory();//  加载历史聊天记录
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
//    ui->sayButton->setText("发送私聊");
    // 进入私聊时
    m_privateTarget = username;
    ui->sayButton->setText("发送私聊");
    ui->saylineEdit->setPlaceholderText(QString("私聊 → %1").arg(m_privateTarget));
    ui->exitPrivateButton->setEnabled(true); // 👈 显示退出按钮
}
//新增
void MainWindow::on_exitPrivateButton_clicked()
{
    m_privateTarget.clear();
    ui->sayButton->setText("发送");
    ui->saylineEdit->setPlaceholderText("");
    ui->roomtextEdit->append("[已退出私聊，返回群聊]");
    ui->exitPrivateButton->setEnabled(false);
}

void MainWindow::initDatabase()
{
    // 使用本地路径（避免依赖特定目录）
    QString dbPath = QCoreApplication::applicationDirPath() + "/Lab5a.db";

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "无法打开数据库：" << m_db.lastError().text();
        return;
    }

    else qDebug() << " 数据库连接成功：Lab5a.db";
}
void MainWindow::saveMessage(const QString &type, const QString &sender,
                             const QString &receiver, const QString &content)
{
    if (!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO messages (type, sender, receiver, content, timestamp) "
                  "VALUES (:type, :sender, :receiver, :content, datetime('now'))");

    query.bindValue(":type", type);
    query.bindValue(":sender", sender);
    query.bindValue(":receiver", receiver.isEmpty() ? QVariant() : receiver);      query.bindValue(":content", content);

    if (!query.exec()) {
        qWarning() << "保存消息失败：" << query.lastError().text();
    } else {
        qDebug() << "消息保存成功";
    }
}
void MainWindow::saveUserLogin(const QString &nickname)
{
    if (!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.prepare("INSERT OR REPLACE INTO users (nick_name, login_time, logout_time, status) "
                  "VALUES (?, datetime('now'), '', 'online')");
    query.addBindValue(nickname);
    query.exec();
}

void MainWindow::saveUserLogout(const QString &nickname)
{
    if (!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.prepare("UPDATE users SET logout_time = datetime('now'), status = 'offline' "
                  "WHERE nick_name = ?");
    query.addBindValue(nickname);
    query.exec();
}


void MainWindow::loadHistory()
{
    if (!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.exec("SELECT type, sender, receiver, content, timestamp FROM messages "
               "ORDER BY timestamp DESC LIMIT 100");

    QStringList history;
    while (query.next()) {
        QString type = query.value("type").toString();
        QString sender = query.value("sender").toString();
        QString receiver = query.value("receiver").toString();
        QString content = query.value("content").toString();
        QString ts = query.value("timestamp").toDateTime().toString("MM-dd hh:mm");

        if (type == "group") {
            history.prepend(QString("[%1 %2] %3").arg(ts, sender, content));
        } else {
            QString direction = (sender == m_myUsername) ? "→" : "←";
            QString peer = (sender == m_myUsername) ? receiver : sender;
            history.prepend(QString("[%1][私聊 %2 %3] %4")
                            .arg(ts, direction, peer, content));
        }
    }

    ui->roomtextEdit->setPlainText(history.join("\n"));
}
