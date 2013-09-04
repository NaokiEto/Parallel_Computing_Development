This directory attempts to do MPI+Pthreads on vtkContourFilter using the 
following steps:

get the VTK rectilinear files, assign the appropriate numbered file to the
appropriate pthreads+MPI combination. The pthread reads the file and 
passes the data through vtkContourFilter. vtk polydata is outputted and 
then sent to the parent pthread. The parent pthread then conglomerates all 
the data into 1 vtk polydata file. The parent pthread then sends this data 
to the parent processor which conglomerates all of the data from the 
parent pthreads into 1 vtk polydata file. The time is printfed into the 
command line terminal.

To run this program without bash script, we can do

mpirun -np "$NUMPROCESSES" ./build/ApplyingVtkContourFilter "$NUMTHREADS" "$FILENAMEVTK" "$PREFIX"

So, for example,

mpirun -np 3 ./build/ApplyingVtkContourFilter 8 AllStars.vtk 27noise.vtk.

(there would be 3 total processor, so 2 child processors and 1 main processor

there are 8 total threads here, each thread takes a file, unlike MPI,
where the main processor does not)


To run this program with the bash script, we can do

sh LetsBashBig.sh 3 8 AllStars.vtk 27noise.vtk. 10

which would repeat the program 10 times
