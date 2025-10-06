#pragma once

#include "graph.h"
#include "network_manager.h"
#include "osm_loader.h"
#include <QMouseEvent>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QWheelEvent>

class MapWidget : public QOpenGLWidget, protected QOpenGLFunctions {
  Q_OBJECT

public:
  MapWidget(QWidget *parent = nullptr);

protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

  void drawRoads();
  void drawPolygons();
  void drawBuildings();
  void wheelEvent(QWheelEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void drawRoadNames(QPainter &painter);
  void drawAreaNames(QPainter &painter);
  QPointF mapToScreen(const QPointF &geo);
  QPointF projectLonLat(double lon, double lat);

private:
  Graph graph;
  OSMLoader *loader;
  NetworkManager *net;
  float zoom = 1.0f;
  float panX = 0, panY = 0;
  QPoint lastMousePos;
  bool isDragging = false;
  Qt::MouseButton dragButton;
  float mapWidth = 0;
  float mapHeight = 0;
  void drawBackground();
};
