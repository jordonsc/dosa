#!/usr/bin/env python3

import argparse
from kivy.config import Config
from dosa.grid_ui import GridApp

# Arg parser
parser = argparse.ArgumentParser(description='Grid UI')
args = parser.parse_args()


# Main app
def run_app():
    Config.set('graphics', 'width', '800')
    Config.set('graphics', 'height', '480')

    GridApp().run()


if __name__ == "__main__":
    run_app()
