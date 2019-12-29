# -*- coding: utf-8 -*-

import matplotlib.dates as mdates
from matplotlib import rcParams
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg
from matplotlib.figure import Figure
from matplotlib.widgets import RectangleSelector
from PySide2.QtCore import Signal
from PySide2.QtWidgets import QSizePolicy

rcParams["font.size"] = 12
rcParams["legend.fontsize"] = "small"
rcParams["figure.titlesize"] = "medium"
rcParams["xtick.labelsize"] = "small"
rcParams["ytick.labelsize"] = "small"
rcParams["markers.fillstyle"] = "none"


class Plot(FigureCanvasQTAgg):
    mouse_motion = Signal(float, float)

    def __init__(self, parent=None):
        self.fig = Figure()

        self.fig.autofmt_xdate()

        FigureCanvasQTAgg.__init__(self, self.fig)

        self.setParent(parent)

        self.axes = self.fig.add_subplot(111)

        self.axes.tick_params(direction="in")

        size_policy = QSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)

        self.setSizePolicy(size_policy)

        self.markers = ("o", "s", "v", "P", "*", "D", "x", ">")
        self.colors = ("#2196f3", "#f44336", "#4caf50", "#ff9800", "#607d8b", "#673ab7", "#795548")

        self.rectangle = RectangleSelector(self.axes, self.rectangle_callback,
                                           drawtype='box', useblit=True,
                                           button=[1],  # only left button
                                           minspanx=5, minspany=5,
                                           spancoords='pixels',
                                           rectprops=dict(facecolor='#ffc400', edgecolor='black',
                                                          alpha=0.2, fill=True),
                                           interactive=False)

        self.fig.canvas.mpl_connect('motion_notify_event', self.on_mouse_motion)

    def plot(self, x, y, marker_idx):
        line_obj, = self.axes.plot(x, y, color=self.colors[marker_idx], linestyle="-")

        return line_obj

    def plot_date(self, x, y, marker_idx, label):
        x = mdates.epoch2num(x)

        line_obj, = self.axes.plot(x, y, color=self.colors[marker_idx], linestyle="-", label=label)

        # self.axes.fill_between(x, y, alpha=0.4)

        return line_obj

    def set_xlabel(self, value):
        self.axes.set_xlabel(value)

    def set_ylabel(self, value):
        self.axes.set_ylabel(value)

    def set_title(self, value):
        self.axes.set_title(value)

    def set_grid(self, value):
        if value:
            self.axes.grid(b=value, linestyle="--")
        else:
            self.axes.grid(b=value)

    def set_margins(self, value):
        self.axes.margins(value)

    def tight_layout(self):
        self.fig.tight_layout()

    def redraw_canvas(self):
        self.axes.relim()
        self.axes.autoscale_view(tight=True)

        self.axes.xaxis.set_major_formatter(mdates.DateFormatter('%d/%m/%Y'))

        self.fig.autofmt_xdate()
        self.fig.tight_layout()

        self.axes.legend().set_draggable(True)  # has to be called after tight_layout

        self.draw_idle()

    def rectangle_callback(self, press_event, release_event):
        x1_data, y1_data = press_event.xdata, press_event.ydata
        x2_data, y2_data = release_event.xdata, release_event.ydata

        self.axes.set_xlim(x1_data, x2_data)
        self.axes.set_ylim(y1_data, y2_data)

        self.draw_idle()

    def save_image(self, path):
        self.fig.savefig(path)

    def on_mouse_motion(self, event):
        x_data, y_data = event.xdata, event.ydata

        self.mouse_motion.emit(x_data, y_data)

    def num2date(self, value):
        return mdates.num2date(value)
