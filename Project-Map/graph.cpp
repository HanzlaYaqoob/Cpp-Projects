#include "graph.h"

// QPointF Graph::normalizeLatLon(double lat, double lon) {
//   double x = (lon - minLon) * scale;
//   double y = (maxLat - lat) * scale;
//   return QPointF(x, y);
// }

void Graph::normalizeCoordinates() {
  if (buildings.empty())
    return;

  // Step 1: Find bounds
  minLon = 1e9;
  maxLon = -1e9;
  minLat = 1e9;
  maxLat = -1e9;

  for (const auto &poly : buildings) {
    for (const auto &pt : poly.nodes) {
      double lon = pt.x();
      double lat = pt.y();
      minLon = std::min(minLon, lon);
      maxLon = std::max(maxLon, lon);
      minLat = std::min(minLat, lat);
      maxLat = std::max(maxLat, lat);
    }
  }

  // Step 2: Compute and store scale
  double width = maxLon - minLon;
  double height = maxLat - minLat;

  scale = 1000.0 / std::max(width, height); // <- this is now saved
                                            // Fit into ~1000x1000
  centerX = ((maxLon - minLon) * scale) / 2.0;
  centerY = ((maxLat - minLat) * scale) / 2.0;

  // Step 3: Normalize all building points
  for (auto &poly : buildings) {
    for (auto &pt : poly.nodes) {
      double normX = (pt.x() - minLon) * scale;
      double normY = (maxLat - pt.y()) * (-scale); // Y flipped (top-down)
      pt.setX(normX);
      pt.setY(normY);
    }
  }

  for (auto &poly : polygons) {
    for (auto &pt : poly.nodes) {
      double normX = (pt.x() - minLon) * scale;
      double normY = (maxLat - pt.y()) * (-scale);
      pt.setX(normX);
      pt.setY(normY);
    }
  }

  for (auto &road : roads) {
    for (auto &pt : road.nodes) {
      double normX = (pt.x() - minLon) * scale;
      double normY = (maxLat - pt.y()) * (-scale);
      pt.setX(normX);
      pt.setY(normY);
    }
  }

  for (auto &edge : edges) {
    for (auto &pt : edge.geometry) {
      double normX = (pt.x() - minLon) * scale;
      double normY = (maxLat - pt.y()) * (-scale);
      pt.setX(normX);
      pt.setY(normY);
    }
  }
}
