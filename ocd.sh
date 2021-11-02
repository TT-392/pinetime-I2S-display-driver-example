#!/bin/bash

openocd_install_location=$HOME"/applications/openocd"

if [ "$1" = "reset" ];
then
    $openocd_install_location/src/openocd -s $openocd_install_location/tcl -f test.cfg -c "init" -c "reset" -c "exit"
fi

if [ "$1" = "flash" ];
then
    $openocd_install_location/src/openocd -s $openocd_install_location/tcl -f flash.cfg
fi

if [ "$1" = "makeflash" ];
then
    touch src/utils/compile_info.c
    make -f Makefile_raw EXTRAFLAGS=-DstableDisplay
    $openocd_install_location/src/openocd -s $openocd_install_location/tcl -f flash.cfg -l /tmp/openocd.log
fi

if [ "$1" = "flashmanual" ];
then
    $openocd_install_location/src/openocd -s $openocd_install_location/tcl -f flashmanual.cfg
fi
