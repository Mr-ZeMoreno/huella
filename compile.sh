#!/bin/bash

meson compile -C build

cp ./build/*.c ./so/
