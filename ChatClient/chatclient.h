#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>

class chatClient : public QObject
{
    Q_OBJECT
public:
    explicit chatClient(QObject *parent = nullptr);
    //新增
    void sendJson(const QJsonObject &json);

signals:
    void connected();
    void messageReceived(const QString &text);
    void jsonReceived(const QJsonObject &docObj);
    //新增
    void disconnected();

private:
    QTcpSocket *m_clientSocket;

public slots:
    void onReadyRead();
//    void sendMessage(const QString &text,const QString &type = "message");
    void connectToServer(const QHostAddress &address, quint16 port);
    void disconnectFromHost();

};

#endif // CHATCLIENT_H
