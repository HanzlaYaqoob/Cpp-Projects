#pragma once
#include "graph.h"
#include <QJsonDocument>
#include <QObject>

class OSMLoader : public QObject {
  Q_OBJECT

public:
  explicit OSMLoader(Graph &graph, QObject *parent = nullptr);
  void loadAreasFromJSON(const QByteArray &jsonData);
  void detectTwinEdgesWithSameName();

private:
  Graph &graph;
};
