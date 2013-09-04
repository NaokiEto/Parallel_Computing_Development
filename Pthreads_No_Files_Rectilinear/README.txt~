This directory attempts to do Pthreads on vtkContourFilter using the 
following steps:

get the VTK rectilinear files, assign the appropriate numbered file to the
appropriate pthreads. The pthread reads the file and passes the
data through vtkContourFilter. vtk polydata is outputted and then written 
to a vtkpolydata temporary file. The file is then sent to the parent 
pthread which is waiting this whole time for the child pthreads to send
the files. The parent pthread then conglomerates all the files into 1 
vtk polydata file.

To run this program without bash script, we can do

./build/ApplyingVtkContourFilter "$NUMTHREADS" "$FILENAMEVTK" "$PREFIX"

So, for example,

./build/ApplyingVtkContourFilter 8 AllStars.vtk 27noise.vtk.

(there are 8 total threads here)


To run this program with the bash script, we can do

sh LetsBashBig.sh 8 AllStars.vtk 27noise.vtk. 10

which would repeat the program 10 times
