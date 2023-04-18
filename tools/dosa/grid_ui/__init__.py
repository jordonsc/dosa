from kivy.app import App
from kivy.uix.widget import Widget


class GridWidget(Widget):
    pass


class GridApp(App):
    def build(self):
        return GridWidget()
