#!/bin/bash

#------------IMPORTANT------------#
#--RUN THIS SCRIPT BEFOR COMMIT---#
#---------------------------------#

#usage:
#

board=m8_k200_v1_config

if [ "$1" == "m6v1" ]; then
    board=m6_mbx_v1_config
elif [ "$1" == "m6v2" ]; then
    board=m6_mbx_v2_config
fi

make distclean && make $board && make -j8 || make -j8

