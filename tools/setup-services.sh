#!/usr/bin/env bash

app=$(python3 -c "import os; print(os.path.dirname(os.path.realpath(\"$0\")))")

cp -f ${app}/systemd/* /usr/lib/systemd/system && \
systemctl daemon-reload && \
systemctl enable grid && \
systemctl enable gridui
