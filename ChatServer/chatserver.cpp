#include "chatserver.h"
#include "serverworker.h"
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>  // 添加这个头文件



ChatServer::ChatServer(QObject *parent):
    QTcpServer(parent)
{

}

// 添加检查用户名是否重复的方法
bool ChatServer::isUsernameTaken(const QString &username)
{

    for (ServerWorker *worker : m_clients) {
        if (worker->userName() == username) {
            return true;  // 用户名已存在
        }
    }
    return false;  // 用户名可用
}

void ChatServer::incomingConnection(qintptr socketDescriptor)
{
    ServerWorker *worker = new ServerWorker(this);
    if (!worker->setSocketDescriptor(socketDescriptor)) {
        worker->deleteLater();
        return;
    }
    connect(worker, &ServerWorker::logMessage, this, &ChatServer::logMessage);
    connect(worker, &ServerWorker::jsonReceived, this, &ChatServer::jsonReceived);
    connect(worker, &ServerWorker::disconnectedFromClient, this, std::bind(&ChatServer::userDisconnected, this, worker));



    m_clients.append(worker);
    emit logMessage("新的用户连接上了");
}

void ChatServer::broadcast(const QJsonObject &message, ServerWorker *exclude)
{
    for (ServerWorker *worker : m_clients) {
        worker->sendJson(message);
    }


}

void ChatServer::stopServer()
{
    close();
}

void ChatServer::jsonReceived(ServerWorker *sender, const QJsonObject &docObj)
{
    const QJsonValue typeVal = docObj.value("type");
    if (typeVal.isNull() || !typeVal.isString())
        return;
    if (typeVal.toString().compare("message", Qt::CaseInsensitive) == 0) {
        const QJsonValue textVal = docObj.value("text");
        if (textVal.isNull() || !textVal.isString())
            return;
        const QString text = textVal.toString().trimmed();
        if (text.isEmpty())
            return;
        QJsonObject message;
        message["type"] = "message";
        message["text"] = text;
        message["sender"] = sender->userName();

        broadcast(message, sender);
    } else if (typeVal.toString().compare("login", Qt::CaseInsensitive) == 0) {
        const QJsonValue usernameVal = docObj.value("text");
        if (usernameVal.isNull() || !usernameVal.isString())
            return;

        const QString username = usernameVal.toString().trimmed();
        if (username.isEmpty()) {
            // 用户名为空，发送错误信息
            QJsonObject errorMessage;
            errorMessage["type"] = "loginError";
            errorMessage["text"] = "用户名不能为空";
            sender->sendJson(errorMessage);
            return;
        }

        // 检查用户名是否已存在
        if (isUsernameTaken(username)) {
            // 用户名重复，发送错误信息
            QJsonObject errorMessage;
            errorMessage["type"] = "loginError";
            errorMessage["text"] = "用户名已存在，请选择其他用户名";
            sender->sendJson(errorMessage);

            // 在服务器控制台输出错误信息
            qDebug() << "登录失败：用户名" << username << "已存在";
            emit logMessage(QString("登录失败：用户名%1已存在").arg(username));
            return;
        }

        sender->setUserName(username);
        QJsonObject connectedMessage;
        connectedMessage["type"] = "newuser";
        connectedMessage["username"] = username;
        broadcast(connectedMessage, sender);

        QJsonObject userListMessage;
        userListMessage["type"] = "userlist";
        QJsonArray userlist;



        for (ServerWorker *worker : m_clients) {
            if (worker == sender)
                userlist.append(worker->userName() + "*");
            else
                userlist.append(worker->userName());
        }
        userListMessage["userlist"] = userlist;
        sender->sendJson(userListMessage);

        // 在服务器控制台输出成功信息
        qDebug() << "用户" << username << "登录成功";
        emit logMessage(QString("用户%1登录成功").arg(username));
    }

    //新增
    // 在 chatserver.cpp 的 jsonReceived 函数中，else if (login) 之后添加：
    else if (typeVal.toString().compare("private", Qt::CaseInsensitive) == 0) {
        const QJsonValue targetVal = docObj.value("target");
        const QJsonValue textVal = docObj.value("text");

        if (targetVal.isNull() || !targetVal.isString() ||
                textVal.isNull() || !textVal.isString()) {
            return;
        }

        QString target = targetVal.toString().trimmed();
        QString text = textVal.toString().trimmed();

        if (target.isEmpty() || text.isEmpty()) {
            return;
        }

        // 查找目标用户
        ServerWorker *targetWorker = nullptr;
        for (ServerWorker *worker : m_clients) {
            if (worker->userName() == target) {
                targetWorker = worker;
                break;
            }
        }

        if (targetWorker) {
            // 构造私聊消息
            QJsonObject privateMsg;
            privateMsg["type"] = "private";
            privateMsg["sender"] = sender->userName();
            privateMsg["text"] = text;

            // 只发给目标用户
            targetWorker->sendJson(privateMsg);

            // 可选：也回显给发送者（已在客户端做了，可省略）
            // sender->sendJson(privateMsg);
        } else {
            // 目标用户不存在，可选：通知发送者
            QJsonObject error;
            error["type"] = "privateError";
            error["text"] = QString("用户 %1 不在线").arg(target);
            sender->sendJson(error);
        }
    }
}

void ChatServer::userDisconnected(ServerWorker *sender)
{
    m_clients.removeAll(sender);
    const QString userName = sender->userName();


    if (!userName.isEmpty()) {
        QJsonObject disconnectedMessage;
        disconnectedMessage["type"] = "userdisconnected";
        disconnectedMessage["username"] = userName;
        broadcast(disconnectedMessage, nullptr);
        emit logMessage(userName + " disconnected");
    }
    sender->deleteLater();
}
