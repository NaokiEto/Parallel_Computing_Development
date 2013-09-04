This directory attempts to do MPI on vtkContourFilter using the following
steps:

get the VTK rectilinear files, assign the appropriate numbered file to the
appropriate child processor. Child processor reads the file and passes the
data through vtkContourFilter. vtk polydata is outputted and then sent to 
the parent processor which is waiting this whole time for the child 
processor to send the data. The parent processor then conglomerates all 
the data into 1 vtk polydata file. The time is outputted into a file.

To run this program without bash script, we can do

mpirun -np "$NUMPROCESSES" ./build/ApplyingVtkContourFilter "$FILENAMEVTK" "$PREFIX"

So, for example,

mpirun -np 9 ./build/ApplyingVtkContourFilter AllStars.vtk 27noise.vtk.

which includes the master process (so there are 8 child processes in this example)


To run this program with the bash script, we can do

sh LetsBashBig.sh 9 AllStars.vtk 27noise.vtk. 10

which would repeat the program 10 times
