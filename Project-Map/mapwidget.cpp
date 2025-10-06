#include "mapwidget.h"
#include <QDebug>
#include <QOpenGLFunctions>
#include <QPainter>
#include <algorithm>
#include <cmath>

MapWidget::MapWidget(QWidget *parent) : QOpenGLWidget(parent) {
  setMouseTracking(true);
  loader = new OSMLoader(graph, this);
  net = new NetworkManager(this);

  connect(net, &NetworkManager::dataReceived, this,
          [=](const QByteArray &data) {
            loader->loadAreasFromJSON(data);
            qDebug() << "Total roads:" << graph.roads.size();

            qDebug() << "Area label size:" << graph.areaLabels.size();
            // for (const AreaLabel &label : graph.areaLabels) {
            //   qDebug() << "Label:" << QString::fromStdString(label.name);
            // }

            mapWidth = (graph.maxLon - graph.minLon) * graph.scale;
            mapHeight = (graph.maxLat - graph.minLat) * graph.scale;

            float centerLon = (graph.minLon + graph.maxLon) / 2.0;
            float centerLat = (graph.minLat + graph.maxLat) / 2.0;

            float centerX = (centerLon - graph.minLon) * graph.scale;
            float centerY = (centerLat - graph.minLat) * graph.scale;

            panX = -centerX + width() / (2.0f * zoom);
            panY = -centerY + height() / (2.0f * zoom);

            update();
          });

  double south = 32.09;
  double north = 32.21;
  double west = 74.13;
  double east = 74.24;

  QString query = QString(R"(
[out:json];
(
    node["place"~"^(suburb|neighbourhood|quarter)$"](%1,%2,%3,%4);
  way["landuse"="residential"]["name"](%1,%2,%3,%4);
  relation["landuse"="residential"]["name"](%1,%2,%3,%4);
  relation["boundary"="administrative"]["name"](%1,%2,%3,%4);
    way["building"](%1,%2,%3,%4);
    relation["building"](%1,%2,%3,%4);
    way["landuse"](%1,%2,%3,%4);
    way["polygon"](%1,%2,%3,%4);
    way["leisure"](%1,%2,%3,%4);
    way["highway"](%1,%2,%3,%4);
);
out body;
>;
out skel qt;
)")
                      .arg(south)
                      .arg(west)
                      .arg(north)
                      .arg(east);

  net->fetchOverpassData(query);
}

void MapWidget::initializeGL() {
  glMatrixMode(GL_MODELVIEW);
  initializeOpenGLFunctions();
  glClearColor(1, 1, 1, 1); // White background

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-width() / 2, width() / 2, -height() / 2, height() / 2, -1, 1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void MapWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Set background color
    QColor bgColor("#E0DFDF");
    glColor3f(bgColor.redF(), bgColor.greenF(), bgColor.blueF());
    glBegin(GL_QUADS);
    glVertex2f(-width(), -height());
    glVertex2f(width(), -height());
    glVertex2f(width(), height());
    glVertex2f(-width(), height());
    glEnd();

    // ✅ Tell Qt we're doing OpenGL manually
    QPainter painter(this);
    painter.beginNativePainting();   // <-- Important!

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glScalef(zoom, zoom, 1.0f);
    glTranslatef(panX, panY, 0);

    drawBuildings();
    drawPolygons();
    drawRoads();

    glFlush();

    painter.endNativePainting();     // <-- Restore Qt state after OpenGL

    // ✅ Now safe to draw labels, text, etc. using QPainter
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    QTransform transform;
    transform.translate(width() / 2.0, height() / 2.0);
    transform.scale(zoom, -zoom);
    transform.translate(panX, panY);
    painter.setTransform(transform);

    drawAreaNames(painter);
    drawRoadNames(painter);
}


void MapWidget::drawRoads() {
  QStringList layerOrder = {"footway",     "path",         "service",
                            "residential", "unclassified", "tertiary",
                            "secondary",   "trunk",        "primary",
                            "highway",     "motorway"};

  for (const QString &layerType : layerOrder) {
    for (const Road &road : graph.roads) {
      if (road.nodes.size() < 2)
        continue;

      if (road.type != layerType)
        continue;

      QColor baseColor = Qt::black;
      QColor borderColor = Qt::black;
      float width = 1.0f;
      float borderWidth = 1.2f;
      float opacity = 1.0f;
      const QString &type = road.type;

      // Assign colors & sizes based on type (same as your current logic)
      if (type == "motorway") {
        baseColor = QColor(255, 208, 0);
        borderColor = QColor(0, 0, 0);
        width = 6.0f;
        borderWidth = 7.0f;

      } else if (type == "primary" || type == "highway") {
        baseColor = QColor(255, 165, 0);
        borderColor = QColor(0, 0, 0);
        width = 6.0f;
        borderWidth = 7.0f;
      } else if (type == "secondary") {
        baseColor = QColor(255, 220, 120);
        borderColor = QColor(0, 0, 0);
        width = 6.0f;
        borderWidth = 7.0f;
      } else if (type == "tertiary") {
        baseColor = QColor(255, 220, 120);
        borderColor = QColor(0, 0, 0);
        width = 6.0f;
        borderWidth = 7.5f;
      } else if (type == "trunk") {
        baseColor = QColor(255, 165, 0);
        borderColor = QColor(0, 0, 0);
        width = 6.0f;
        borderWidth = 7.0f;

      } else if (type == "residential" || type == "unclassified" ||
                 type == "service") {

        baseColor = QColor("#FFFFFF");   // Light gray fill
        borderColor = QColor("#4A4A4A"); // Dark gray border

        width = 1.9f;
        borderWidth = 2.0f;
      } else if (type == "footway" || type == "path") {
        baseColor = QColor(120, 200, 120, 140);
        borderColor = QColor(50, 100, 50);
        width = 1.0f;
        borderWidth = 1.8f;
      }

      // Border Pass
      glColor4f(borderColor.redF(), borderColor.greenF(), borderColor.blueF(),
                opacity);
      glLineWidth(borderWidth);
      glBegin(GL_LINE_STRIP);
      for (const QPointF &pt : road.nodes)
        glVertex2f(pt.x(), pt.y());
      glEnd();

      // Main Road Pass
      glColor4f(baseColor.redF(), baseColor.greenF(), baseColor.blueF(),
                opacity);
      glLineWidth(width);
      glBegin(GL_LINE_STRIP);
      for (const QPointF &pt : road.nodes)
        glVertex2f(pt.x(), pt.y());
      glEnd();

      for (const QPointF &pt : road.nodes) {
        glVertex2f(pt.x(), pt.y());
      }
      glEnd();
    }
  }
}

void MapWidget::drawRoadNames(QPainter &painter) {
  QFont font = painter.font();
  font.setPointSizeF(1.0); // Scale with map zoom externally
  painter.setFont(font);
  painter.setPen(Qt::black);

  for (const Road &road : graph.roads) {
    if (road.name.isEmpty() || road.nodes.size() < 2)
      continue;

    const QString &name = road.name;
    const QVector<QPointF> &nodes = road.nodes;

    QFontMetricsF fm(font);

    // Compute road length
    QVector<float> segmentLengths;
    float totalLength = 0.0f;
    for (int i = 1; i < nodes.size(); ++i) {
      float len = QLineF(nodes[i - 1], nodes[i]).length();
      segmentLengths.push_back(len);
      totalLength += len;
    }

    // Total text width
    float textLength = 0.0f;
    for (QChar ch : name)
      textLength += fm.horizontalAdvance(ch);

    // Skip drawing if text is longer than the road
    if (textLength >= totalLength)
      continue;

    // Center the text
    float offset = (totalLength - textLength) / 2.0f;
    float currentOffset = 0.0f;

    int segIndex = 0;
    float segPos = 0.0f;

    for (int i = 0; i < name.size(); ++i) {
      QChar ch = name[i];
      float charWidth = fm.horizontalAdvance(ch);

      float charMidOffset = offset + currentOffset + charWidth / 2;

      // Move to segment where this character's center lies
      while (segIndex < segmentLengths.size() &&
             segPos + segmentLengths[segIndex] < charMidOffset) {
        segPos += segmentLengths[segIndex++];
      }

      if (segIndex >= nodes.size() - 1)
        break;

      QPointF p1 = nodes[segIndex];
      QPointF p2 = nodes[segIndex + 1];
      float segLength = segmentLengths[segIndex];

      float t = (charMidOffset - segPos) / segLength;
      QPointF pos = p1 + (p2 - p1) * t;

      float angle = std::atan2(p2.y() - p1.y(), p2.x() - p1.x()) * 180.0 / M_PI;

      painter.save();
      painter.translate(pos);
      painter.rotate(angle);
      painter.scale(1.0, -1.0); // Flip for OpenGL-compatible coordinate system
      painter.drawText(QPointF(-charWidth / 2.0, 0), QString(ch));
      painter.restore();

      currentOffset += charWidth;
    }
  }
}

QPointF MapWidget::projectLonLat(double lon, double lat) {
  // You should use the same projection params as the rest of your map.
  double x = lon * 111320.0; // Approx meters per degree longitude
  double y = lat * 110540.0; // Approx meters per degree latitude
  return QPointF(x, y);
}

void MapWidget::drawAreaNames(QPainter &painter) {
  for (const AreaLabel &label : graph.areaLabels) {
    if (zoom > 2.0)
      continue;

    QPointF pt = projectLonLat(label.center.x(), label.center.y());

    QFont font = painter.font();
    font.setPointSizeF(label.isMajor ? std::clamp(zoom * 10.0f, 8.0f, 30.0f)
                                     : std::clamp(zoom * 6.0f, 6.0f, 18.0f));
    font.setBold(label.isMajor);
    painter.setFont(font);

    QString name = QString::fromStdString(label.name);
    QFontMetricsF fm(font);
    QRectF textRect = fm.boundingRect(name);
    QPointF centerPt = pt - QPointF(textRect.width() / 2, -fm.ascent() / 2);

    painter.setPen(QPen(Qt::white, 3.0));
    painter.drawText(centerPt, name);

    painter.setPen(Qt::black);
    painter.drawText(centerPt, name);
  }
}

QPointF MapWidget::mapToScreen(const QPointF &geo) {
  QPointF pt = geo;
  pt.setX((pt.x() - graph.minLon) * graph.scale);
  pt.setY((graph.maxLat - pt.y()) * graph.scale);
  pt.setX(pt.x() * zoom + panX * zoom + width() / 2.0f);
  pt.setY(pt.y() * zoom + panY * zoom + height() / 2.0f);
  return pt;
}

void MapWidget::drawPolygons() {
  for (const auto &poly : graph.polygons) {
    if (poly.nodes.size() < 3)
      continue;

    // Determine polygon type
    QString type = poly.tags.value("landuse", poly.tags.value("leisure"));

    // Set color based on type
    if (type == "grass" || type == "meadow")
      glColor3f(0.8f, 1.0f, 0.8f); // green
    else if (type == "forest")
      glColor3f(0.5f, 0.8f, 0.5f); // darker green
    else if (type == "cemetery")
      glColor3f(0.9f, 0.9f, 0.7f);
    else if (type == "residential")
      glColor3f(0.95f, 0.95f, 0.9f);
    else
      glColor3f(0.85f, 0.85f, 0.85f); // default gray

    glBegin(GL_POLYGON);
    for (const auto &pt : poly.nodes)
      glVertex2f(pt.x(), pt.y());
    glEnd();
  }
}

void MapWidget::resizeGL(int w, int h) {
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-w / 2, w / 2, -h / 2, h / 2, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void MapWidget::drawBuildings() {
  glColor3f(0.6f, 0.6f, 0.8f); // Light bluish-gray for buildings

  for (const auto &building : graph.buildings) {
    if (building.nodes.size() < 3)
      continue; // Not a valid polygon

    glBegin(GL_POLYGON);
    for (const auto &pt : building.nodes) {
      glVertex2f(pt.x(), pt.y());
    }
    glEnd();
  }
}

void MapWidget::drawBackground() {
  glColor3f(0.2f, 0.6f, 0.8f); // light blue grid
  glBegin(GL_LINES);
  for (int i = -1000; i <= 1000; i += 100) {
    glVertex2f(i, -1000);
    glVertex2f(i, 1000);
    glVertex2f(-1000, i);
    glVertex2f(1000, i);
  }
  glEnd();
}

void MapWidget::wheelEvent(QWheelEvent *event) {
  float oldZoom = zoom;
  QPoint numDegrees = event->angleDelta() / 8;
  float zoomFactor = 1.0f + numDegrees.y() / 240.0f;
  zoom *= zoomFactor;

  // Clamp zoom
  zoom = std::clamp(zoom, 0.9f, 60.0f);

  // Adjust pan to zoom at mouse position
  float mx = event->position().x() - width() / 2.0f;
  float my = height() / 2.0f - event->position().y(); // Y is flipped

  panX -= mx * (1.0f / oldZoom - 1.0f / zoom);
  panY -= my * (1.0f / oldZoom - 1.0f / zoom);

  update();
}

void MapWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() & Qt::MiddleButton) {
    setCursor(Qt::PointingHandCursor);
    isDragging = true;
    lastMousePos = event->pos();
  }
}

void MapWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() & Qt::MiddleButton) {
    unsetCursor();
    isDragging = false;
  }
}

void MapWidget::mouseMoveEvent(QMouseEvent *event) {
  if (isDragging) {
    QPointF delta = event->pos() - lastMousePos;
    panX += delta.x() / zoom;
    panY -= delta.y() / zoom; // Y flipped in OpenGL

    float minX = -mapWidth;
    float maxX = mapWidth - 555.416f;
    float minY = -mapHeight + 1050.416f;
    float maxY = mapHeight;

    panX = std::clamp(panX, minX, maxX);
    panY = std::clamp(panY, minY, maxY);

    update();
    lastMousePos = event->pos();
  }
}
