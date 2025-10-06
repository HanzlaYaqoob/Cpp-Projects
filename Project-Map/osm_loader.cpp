#include "osm_loader.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

OSMLoader::OSMLoader(Graph &g, QObject *parent) : QObject(parent), graph(g) {}

void OSMLoader::loadAreasFromJSON(const QByteArray &jsonData) {
  QJsonDocument doc = QJsonDocument::fromJson(jsonData);
  if (!doc.isObject())
    return;

  QJsonObject root = doc.object();
  QJsonArray elements = root["elements"].toArray();

  QMap<qint64, Node> tempNodes;
  QMap<qint64, PolygonArea> waysMap;

  // Step 1: Load all nodes
  for (const auto &elVal : elements) {
    QJsonObject el = elVal.toObject();
    if (el["type"].toString() == "node") {
      qint64 id = el["id"].toVariant().toLongLong();
      double lat = el["lat"].toDouble();
      double lon = el["lon"].toDouble();
      Node node = {id, lat, lon, {}};
      tempNodes[id] = node;
    }
  }

  // Step 2: Load ways (roads + polygons)
  for (const auto &elVal : elements) {
    QJsonObject el = elVal.toObject();
    QJsonValueRef eltags = el["type"];

    if (el["type"].toString() != "way") {
      continue;
    }

    qint64 id = el["id"].toVariant().toLongLong();
    QJsonArray nds = el["nodes"].toArray();
    QJsonObject tags = el["tags"].toObject();

    // ✅ Store road
    if (tags.contains("highway")) {
      Road road;
      road.id = id;
      road.type = tags["highway"].toString();
      if (tags.contains("name")) {
        road.name = tags["name"].toString();
      }

      for (const auto &nd : nds) {
        qint64 ref = nd.toVariant().toLongLong();
        if (tempNodes.contains(ref))
          road.nodes.append(QPointF(tempNodes[ref].lon, tempNodes[ref].lat));
      }

      graph.roads.append(road);

      // Add edges between each pair of consecutive nodes
      for (int i = 1; i < nds.size(); ++i) {
        qint64 fromId = nds[i - 1].toVariant().toLongLong();
        qint64 toId = nds[i].toVariant().toLongLong();

        if (!tempNodes.contains(fromId) || !tempNodes.contains(toId))
          continue;

        Node fromNode = tempNodes[fromId];
        Node toNode = tempNodes[toId];

        Edge edge;
        edge.from = fromId;
        edge.to = toId;
        edge.name =
            tags.contains("name") ? tags["name"].toString().toStdString() : "";
        edge.highwayType = tags["highway"].toString().toStdString();
        edge.oneway =
            tags.contains("oneway") && tags["oneway"].toString() == "yes";

        // Set geometry for label drawing
        edge.geometry.push_back(QPointF(fromNode.lon, fromNode.lat));
        edge.geometry.push_back(QPointF(toNode.lon, toNode.lat));

        graph.edges.push_back(edge);
      }
    }

    // ✅ Store building/landuse as polygon
    PolygonArea poly;
    poly.id = id;
    for (auto it = tags.begin(); it != tags.end(); ++it)
      poly.tags[it.key()] = it.value().toString();

    for (const auto &nd : nds) {
      qint64 ref = nd.toVariant().toLongLong();
      if (tempNodes.contains(ref))
        poly.nodes.append(QPointF(tempNodes[ref].lon, tempNodes[ref].lat));
    }

    // Store in appropriate list
    if (tags.contains("building"))
      graph.buildings.append(poly);
    else if (!tags.contains("highway"))
      graph.polygons.append(poly);

    waysMap[id] = poly;
  }

  // Step 3: Handle multipolygon relations (complex buildings)
  for (const auto &elVal : elements) {
    QJsonObject el = elVal.toObject();
    if (el["type"].toString() != "relation")
      continue;

    QJsonObject tags = el["tags"].toObject();
    if (tags.value("type").toString() != "multipolygon")
      continue;

    if (!tags.contains("building"))
      continue; // only buildings for now

    QJsonArray members = el["members"].toArray();
    PolygonArea merged;
    for (auto it = tags.begin(); it != tags.end(); ++it)
      merged.tags[it.key()] = it.value().toString();

    for (const auto &memberVal : members) {
      QJsonObject member = memberVal.toObject();
      if (member["type"] != "way" || member["role"] != "outer")
        continue;

      qint64 wayId = member["ref"].toVariant().toLongLong();
      if (waysMap.contains(wayId)) {
        // Merge way polygon
        for (const QPointF &pt : waysMap[wayId].nodes)
          merged.nodes.append(pt);
      }
    }

    if (!merged.nodes.isEmpty())
      graph.buildings.append(merged);
  }

  // Step 4: Add nodes to main graph
  for (const auto &n : tempNodes)
    graph.nodes[n.id] = n;

  graph.normalizeCoordinates();

  // Step 5: Extract actual area/place names
  for (const auto &elVal : elements) {
    QJsonObject el = elVal.toObject();

    if (el["type"].toString() != "node")
      continue;

    QJsonObject tags = el["tags"].toObject();
    if (!tags.contains("place") || !tags.contains("name"))
      continue;

    QString placeType = tags["place"].toString();
    QString name = tags["name"].toString();

    if (placeType == "neighbourhood" || placeType == "suburb" ||
        placeType == "quarter" || placeType == "city_block") {

      double lat = el["lat"].toDouble();
      double lon = el["lon"].toDouble();

      AreaLabel label;
      label.name = name.toStdString();
      label.center = QPointF(lon, lat);
      label.isMajor = (placeType == "suburb" || placeType == "quarter");

      graph.areaLabels.push_back(label);
    }
  }
}
