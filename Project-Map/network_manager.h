#pragma once
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>

class NetworkManager : public QObject {
  Q_OBJECT

public:
  explicit NetworkManager(QObject *parent = nullptr);
  void fetchOverpassData(const QString &query);

signals:
  void dataReceived(const QByteArray &data);

private slots:
  void handleNetworkReply(QNetworkReply *reply);

private:
  QNetworkAccessManager *manager;
};
