#!/usr/bin/env python3

import argparse
import logging

from kivy.config import Config as KivyConfig
from dosa.grid_ui import GridApp
from dosa.power import is_production_machine

# Arg parser
parser = argparse.ArgumentParser(description='Grid UI')
args = parser.parse_args()


# Main app
def run_app():
    KivyConfig.set('graphics', 'width', '800')
    KivyConfig.set('graphics', 'height', '480')
    KivyConfig.set('input', 'mouse', 'mouse,multitouch_on_demand')

    if is_production_machine():
        KivyConfig.set('graphics', 'show_cursor', '0')

    GridApp().run()


if __name__ == "__main__":
    if not is_production_machine():
        logging.basicConfig()
        logging.getLogger().setLevel(logging.DEBUG)
        logging.debug("Set log level to DEBUG")

    run_app()
