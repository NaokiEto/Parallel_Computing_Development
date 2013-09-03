#!/bin/bash

# Number of total threads          
NUMTHREADS=$1

# File name of VTK produced
FILENAMEVTK=$2

# Prefix of filenames
PREFIX=$3

# Number of times the program is to be ran
NUMREPETITIONS=$4

# Just parts of file names
PTHREADS="Big_Pthreads"
TRIAL="Threads_TrialNumber"
ZERO="0"

# Make a directory of this particular amount of threads
mkdir $NUMTHREADS$PTHREADS
cd $NUMTHREADS$PTHREADS

# Do the program the amount of desired repeated times
for i in $(seq 1 $NUMREPETITIONS)
    do
        # Make the directory for the particular trial number
        mkdir $PTHREADS$NUMTHREADS$TRIAL$ZERO$i
        
        # Go back to the Pthreads_Files_Real directory
        cd ../

        # Grab vtk files and copy it into the trial number directory
        for f in *.vtk
            do
                cp -v $f $NUMTHREADS$PTHREADS/$PTHREADS$NUMTHREADS$TRIAL$ZERO$i
            done

        # Go to the trial number directory
        cd $NUMTHREADS$PTHREADS/$PTHREADS$NUMTHREADS$TRIAL$ZERO$i

        # Run the code 
        ./../../build/ApplyingVtkContourFilter "$NUMTHREADS" "$FILENAMEVTK" "$PREFIX"

        # Remove the original vtk file
        # rm $f
        
        # Let's go back to the directory for the particular amount of threads and repeat
        cd ../
    done

