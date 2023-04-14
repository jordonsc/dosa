#!/usr/bin/env python3

import argparse
from kivy.config import Config as KivyConfig
from dosa.grid_ui import GridApp

# Arg parser
parser = argparse.ArgumentParser(description='Grid UI')
args = parser.parse_args()


# Main app
def run_app():
    KivyConfig.set('graphics', 'width', '800')
    KivyConfig.set('graphics', 'height', '480')
    KivyConfig.set('graphics', 'show_cursor', '0')

    GridApp().run()


if __name__ == "__main__":
    run_app()
