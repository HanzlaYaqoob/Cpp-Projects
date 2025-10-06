#include "network_manager.h"
#include <QDebug>
#include <QNetworkRequest>
#include <QUrl>

NetworkManager::NetworkManager(QObject *parent) : QObject(parent) {
  manager = new QNetworkAccessManager(this);
  connect(manager, &QNetworkAccessManager::finished, this,
          &NetworkManager::handleNetworkReply);
}

void NetworkManager::fetchOverpassData(const QString &query) {
  QString url = "http://overpass-api.de/api/interpreter?data=" +
                QUrl::toPercentEncoding(query);
  QNetworkRequest request(QUrl{url});
  manager->get(request);
}

void NetworkManager::handleNetworkReply(QNetworkReply *reply) {
  if (reply->error() == QNetworkReply::NoError) {
    emit dataReceived(reply->readAll());
  } else {
    qWarning() << "Network error:" << reply->errorString();
  }
  reply->deleteLater();
}
