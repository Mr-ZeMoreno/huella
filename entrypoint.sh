#!/bin/bash

set -e  # Exit immediately on any error
meson setup --wipe build && \
    meson compile -C build && \
    cp /app/build/*.so /app/py_package/so/ && \
    rm -rf /app/build
