#! /bin/bash

sed -i -e "s/-Wl,-x//g" ./Demo/Makefile ./UnitTest/Makefile ./Tahoe/Makefile

