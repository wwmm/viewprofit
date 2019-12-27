#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys

from PySide2.QtWidgets import QApplication

from ViewProfit.application_window import ApplicationWindow

if __name__ == "__main__":
    APP = QApplication(sys.argv)
    AW = ApplicationWindow()

    sys.exit(APP.exec_())
