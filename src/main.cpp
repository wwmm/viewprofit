#include <QApplication>
#include "main_window.hpp"

auto main(int argc, char* argv[]) -> int {
  QApplication app(argc, argv);

  QCoreApplication::setOrganizationName("wwmm");
  QCoreApplication::setApplicationName("ViewProfit");

  auto mw = MainWindow();

  return QApplication::exec();
}