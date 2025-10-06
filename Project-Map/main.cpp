#include "mapwidget.h"
#include "network_manager.h"
#include "osm_loader.h"
#include <QApplication>
#include <QMainWindow>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  QMainWindow window;
  MapWidget *mapWidget = new MapWidget();

  window.setCentralWidget(mapWidget);
  window.resize(1200, 800);
  window.setWindowTitle("City Map - Gujranwala");
  window.show();

  return app.exec();
}
