#!/bin/bash

meson compile -C build

cp ./build/*.so ./py_package/so/
