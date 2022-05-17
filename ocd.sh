#!/bin/bash

openocd_install_location=$HOME"/applications/openocd"

if [ "$1" = "reset" ];
then
    $openocd_install_location/src/openocd -s $openocd_install_location/tcl -f ocd/bare.cfg -c "init" -c "reset" -c "exit"
fi

if [ "$1" = "flash" ];
then
    $openocd_install_location/src/openocd -s $openocd_install_location/tcl -f ocd/flash.cfg
fi

if [ "$1" = "makeflash" ];
then
    make -j -f Makefile
    $openocd_install_location/src/openocd -s $openocd_install_location/tcl -f ocd/flash.cfg -l /tmp/openocd.log
fi
