#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QVector>      // ✅ 显式包含
#include <QMutex>       // ✅ 用于线程安全
#include "serverworker.h"

class ChatServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit ChatServer(QObject *parent = nullptr);

    // 添加检查用户名是否重复的方法
    bool isUsernameTaken(const QString &username) ;

protected:
    void incomingConnection(qintptr socketDescriptor) override;
    QVector<ServerWorker *>m_clients;

    void broadcast(const QJsonObject &message, ServerWorker *exclude);



signals:
    void logMessage(const QString &msg);

public slots:
    void stopServer();
    void jsonReceived(ServerWorker *sender, const QJsonObject &docObj);
    void userDisconnected(ServerWorker *sender);



};

#endif // CHATSERVER_H
