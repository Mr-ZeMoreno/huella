#!/bin/bash

meson compile -C build

cp ./build/*.so ./so/
