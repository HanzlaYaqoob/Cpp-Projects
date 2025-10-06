#pragma once
#include <QMap>
#include <QPointF>
#include <QString>
#include <QVector>

struct Node {
  qint64 id;
  double lat;
  double lon;
  QPointF screenPos; // Converted position for drawing
};

struct Edge {
  long long from;
  long long to;
  double length;
  std::string name; // NEW: Road name (if available)
  std::string highwayType;
  bool oneway;

  std::vector<QPointF> geometry;
};

struct Road {
  qint64 id;
  QString name;
  QString type;
  QVector<QPointF> nodes;
};

struct PolygonArea {
  qint64 id;
  QVector<QPointF> nodes; // Lat/Lon converted to screen space
  QMap<QString, QString> tags;
};
struct AreaLabel {
  std::string name;
  QPointF center; // in lat/lon
  bool isMajor;
};

class Graph {
public:
  double centerX = 0;
  double centerY = 0;

  double scale = 1.0;
  double minLat = 90.0, maxLat = -90.0;
  double minLon = 180.0, maxLon = -180.0;

  QMap<qint64, Node> nodes;
  QVector<Edge> edges;
  QVector<PolygonArea> buildings;
  QVector<PolygonArea> landuse;
  QList<PolygonArea> polygons;
  QList<Road> roads;
  std::vector<AreaLabel> areaLabels;
  const std::vector<AreaLabel> &getAreas() const { return areaLabels; }

  void normalizeCoordinates(); // Normalize all lat/lon to screen space
};
