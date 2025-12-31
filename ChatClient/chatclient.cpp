#include "chatclient.h"
#include <QDataStream>
#include <QJsonObject>
#include <QJsonDocument>

chatClient::chatClient(QObject *parent)
    : QObject{parent}
{
    m_clientSocket = new QTcpSocket(this);
    connect(m_clientSocket, &QTcpSocket::connected, this, &chatClient::connected);
    connect(m_clientSocket, &QTcpSocket::readyRead, this, &chatClient::onReadyRead);


}

//void chatClient::onReadyRead()
//{
//    QByteArray jsonData;
//    QDataStream socketStream(m_clientSocket);
//    socketStream.setVersion(QDataStream::Qt_5_12);
//    for(;;){
//        socketStream.startTransaction();
//        socketStream >> jsonData;
//        if(socketStream.commitTransaction()){
//            // emit messageReceived(QString::fromUtf8(jsonData));
//            QJsonParseError parseError;
//            const QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData,&parseError);
//            if(parseError.error == QJsonParseError::NoError){
//                if(jsonDoc.isObject()){
//                    // emit logMessage(QJsonDocument(jsonDoc).toJson(QJsonDocument::Compact));
//                    emit jsonReceived(jsonDoc.object());
//                }
//            }
//        }else{
//            break;
//        }
//    }
//}

//修改
void chatClient::onReadyRead()
{
    QDataStream socketStream(m_clientSocket);
    socketStream.setVersion(QDataStream::Qt_5_12);

    while (true) {
        socketStream.startTransaction();
        QByteArray jsonData;
        socketStream >> jsonData;

        if (!socketStream.commitTransaction()) {
            break;
        }

        qDebug() << "【客户端收到】:" << jsonData;

        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
        if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
            emit jsonReceived(jsonDoc.object());
        } else {
            qDebug() << "【JSON解析失败】:" << parseError.errorString();
        }
    }
}

//删除
//void chatClient::sendMessage(const QString &text, const QString &type)
//{
//    if(m_clientSocket->state() != QAbstractSocket::ConnectedState)
//        return;

//    if(!text.isEmpty()){
//        QDataStream serverStream(m_clientSocket);
//        serverStream.setVersion(QDataStream::Qt_5_12);
//        QJsonObject message;
//        message["type"] = type;
//        message["text"] = text;
//        serverStream<< QJsonDocument(message).toJson();
//    }
//}

void chatClient::connectToServer(const QHostAddress &address, quint16 port)
{
    m_clientSocket->connectToHost(address, port);
}

void chatClient::disconnectFromHost()
{
    m_clientSocket->disconnectFromHost();
}


//新增
void chatClient::sendJson(const QJsonObject &json)
{
    if (!m_clientSocket || m_clientSocket->state() != QAbstractSocket::ConnectedState)
        return;

    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);
    qDebug() << "【客户端发送 JSON】:" << data; // 调试日志

    QDataStream stream(m_clientSocket);
    stream.setVersion(QDataStream::Qt_5_12);
    stream << data; // 👈 必须用 QDataStream，和服务端 onReadyRead 匹配
}
