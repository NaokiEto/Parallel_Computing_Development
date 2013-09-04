/**
* Do whatever you want with public license
* Version 2, September 3, 2013
*
* Copyright (C) 2013 Naoki Eto <neto@lbl.gov>
*
* Everyone is permitted to copy and distribute verbatim or modified
* copies of this license document, and changing it is allowed as long
* as the name is changed.
*
* Do whatever you want with the public license
*
* TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
*
* 0. You just do what you want to do.
* 1. Uses VTK_MAJOR_VERSION <= 5
* 
*/
/**
* @file ApplyingVtkContourFilter.cxx
* @author Naoki Eto
* @date September 3, 2013
* @brief This program gets the VTK files, assigns the appropriate VTK
*        rectilinear file to the appropriate pthread (or POSIX thread)/MPI
*        combination, and passes the data through vtkContourFilter, which 
*        outputs the resulting VTK polydata. The data is sent to the parent 
*        thread, which then conglomerates the data into 1 vtk polydata. The
*        parent thread then sends this data to the parent processor, which
*        conglomerates all the data from all the parent pthreads into 1 vtk
*        polydata file. The time is printfed into the command line terminal. 
* @param[in] argv[1] - number of threads for pthreading (look at 
*            README for more information)
* @param[in] argv[2] - the output's filename
* @param[in] argv[3] - the prefix of the files (i.e. 27noise.vtk.)
* @param[out] pWriter - vtkPolyData file with the output's filename
* @return - EXIT_SUCCESS at the end
*/

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPolyDataReader.h>
#include <vtkPointData.h>
#include <vtkPolyDataNormals.h>
#include <vtkPolyDataWriter.h>
#include <vtkAppendPolyData.h>
#include <vtkCleanPolyData.h>
#include <vtkMPIController.h>

#include <mpi.h>//"/home/users/neto/NaokiEto/openmpi-install/include/mpi.h"
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#include <vtkRectilinearGrid.h>
#include <vtkRectilinearGridReader.h>

#include <vtkContourFilter.h>
#include <vtkPoints.h>

/**
 * This struct contains the id of the thread, the rank of the processor, 
 * the filename prefix of the vtk files, the number of total threads, and 
 * vtk poly data that has been outputted by vtkContourFilter. This will 
 * be useful for determining which vtk file goes with which particular 
 * thread/processor combination.
*/
typedef struct Param_Function
{
    int NumThreads;
    const char *VTKinput;
    int ThreadId;
    vtkPolyData* vtkPiece;
    int procRank;
} params;


/**
 * This function takes in the appropriate vtk Rectilinear file and 
 * thread/processor. The thread reads the file and applies vtkContourFilter 
 * to the data. vtk polydata is outputted, and are then sent to the parent 
 * thread. 
*/
void* thread_function(void* ptr)
{
    params* NewPtr;
    NewPtr = (params*) ptr;

    /* The vtk file extension we want to search for */

    int NumOfCharPD = strlen(NewPtr->VTKinput) + 5 + 1;
    int original = NumOfCharPD;

    // figure out how many characters are in the temporary file name, 
    if((NewPtr->procRank-1)*(NewPtr->NumThreads) + NewPtr->ThreadId > 9)
        NumOfCharPD += (int) log10((NewPtr->procRank-1)*(NewPtr->NumThreads) + NewPtr->ThreadId);

    char buf[(int) NumOfCharPD - original + 1];
 
    sprintf(buf, "%d", (NewPtr->procRank-1)*(NewPtr->NumThreads) + NewPtr->ThreadId);

    const char* suffix = ".vtk";

    const char* prefix = NewPtr->VTKinput;

    char prefix_suffix[NumOfCharPD];

    strcpy(prefix_suffix, prefix);

    strcat(prefix_suffix, buf);

    strcat(prefix_suffix, suffix);

    vtkRectilinearGridReader *reader = vtkRectilinearGridReader::New();
    
    reader->SetFileName(prefix_suffix);

    reader->Update();

    // Create a grid
    vtkSmartPointer<vtkRectilinearGrid> grid = reader->GetOutput();
    reader->Delete();

    vtkPointData* pointdata = grid->GetPointData();

    double* range;

    range = pointdata->GetArray("grad")->GetRange();

    vtkContourFilter* contour = vtkContourFilter::New();

    // name of array is "grad"
    contour->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "grad");

    // better than setinput
    contour->SetInputConnection(grid->GetProducerPort());

    // woo 50 contours
    contour->GenerateValues(50, range);

    contour->ComputeNormalsOn();

    contour->Update();

    // calc cell normal
    vtkPolyDataNormals *triangleCellNormals= vtkPolyDataNormals::New();

    triangleCellNormals->SetInputConnection(contour->GetOutputPort());

    triangleCellNormals->ComputeCellNormalsOn();
    triangleCellNormals->ComputePointNormalsOff();
    triangleCellNormals->ConsistencyOn();
    triangleCellNormals->AutoOrientNormalsOn();
    triangleCellNormals->Update(); // creates vtkPolyData

    NewPtr->vtkPiece = triangleCellNormals->GetOutput();

    pointdata->Delete();
}

int main(int argc, char *argv[])
{
    vtkMPIController* controller = vtkMPIController::New();

    double t1 = MPI_Wtime();

    // Initializing MPI
    controller->Initialize(&argc, &argv);

    /* Figure out total amount of processors */
    int MPI_rank = controller->GetLocalProcessId();

    /* Figure out the rank of this processor */
    int MPI_size = controller->GetNumberOfProcesses();

    /* The parent process will be of rank 0 */
    int PARENT = 0;

    int pthreads_size = atoi(argv[1]);

	pthread_t threads[pthreads_size];

    // this is the input into the function for each thread
    params thread_data_array[pthreads_size];

    // If not parent process, do the vtkContourFilter implementation
    if (MPI_rank >= 1)
    {
        //creating threads
        // The file extension to look for is .f.vtk
        std::string fileprefix = argv[3];

	    for (int f = 0; f < pthreads_size; f++) {      
            //creating threads
            thread_data_array[f].NumThreads = pthreads_size;
            thread_data_array[f].procRank = MPI_rank;
            thread_data_array[f].VTKinput = fileprefix.c_str();
            thread_data_array[f].ThreadId = f;
		    pthread_create(&threads[f], NULL, thread_function, (void*)&thread_data_array[f]);
        }

	    for (int j = 0; j < pthreads_size; j++)
        {
		    pthread_join(threads[j], NULL);
        }

        vtkAppendPolyData *appendWriter = vtkAppendPolyData::New();

        for(int y = 0; y < pthreads_size; y++)
        {
            vtkPolyData *inputNum = vtkPolyData::New();

            inputNum->ShallowCopy(thread_data_array[y].vtkPiece);

            appendWriter->AddInput(thread_data_array[y].vtkPiece);

            appendWriter->Update();

            inputNum->Delete();
        }

        // send the vtkPolyData to the parent process
        controller->Send(appendWriter->GetOutput(), 0, 1);
    }

    if (MPI_rank == PARENT)
    {
        // to append each piece into 1 big vtk file
        vtkAppendPolyData *appendWriterPARENT = vtkAppendPolyData::New();

        // go through the processes, and append
        for(int k = 1; k < MPI_size; k++)
        {
            vtkPolyData* pd = vtkPolyData::New();
            controller->Receive(pd, k, 1);

            appendWriterPARENT->AddInput(pd);

            appendWriterPARENT->Update();

            pd->Delete();
        }

        vtkPolyDataWriter *pWriter = vtkPolyDataWriter::New();
        pWriter->SetFileName(argv[2]);

            pWriter->SetInput(appendWriterPARENT->GetOutput());

        pWriter->Write();

        double t2 = MPI_Wtime();

        double time = t2 - t1;

        printf("MPI_Wtime measured the time elapsed to be: %f\n", time);
    }
	MPI_Finalize();

	return EXIT_SUCCESS;
}
