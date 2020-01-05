#include <QApplication>
#include "main_window.hpp"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  auto mw = MainWindow();

  return app.exec();
}