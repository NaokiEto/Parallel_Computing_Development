#!/bin/bash

#VTK file name to convert
PREFIX=$1

#Number of vtk files
NUMFILES=$2

NUMFILES1LESS=$((NUMFILES-1))

#Folder name holding all the vtk files
FOLDER=$3

SUFFIX=".vtk"

# Go through all the vtk files
for n in `seq 0 $NUMFILES1LESS`;
    do
        sed -i '1s/.*/# vtk DataFile Version 2.0/' $FOLDER/$PREFIX$n$SUFFIX

        sed -i '25s/.*/VECTORS grad float/' $FOLDER/$PREFIX$n$SUFFIX

        sed -i '24d' $FOLDER/$PREFIX$n$SUFFIX

        sed -i '5,9d' $FOLDER/$PREFIX$n$SUFFIX

    done
