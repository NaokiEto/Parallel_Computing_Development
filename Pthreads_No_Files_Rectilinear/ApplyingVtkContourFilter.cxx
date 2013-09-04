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
*        rectilinear file to the appropriate pthread (or POSIX thread), 
*        passes the data through vtkContourFilter, which outputs the resulting 
*        VTK polydata. The data is sent to the parent thread, which then 
*        conglomerates the data into 1 vtk polydata file. The time is printfed
*        into the command line terminal. 
* @param[in] argv[1] - number of threads for pthreading (look at 
*            README for more information)
* @param[in] argv[2] - the output's filename
* @param[in] argv[3] - the prefix of the files (i.e. 27noise.vtk.)
* @param[out] pWriter - vtkPolyData file with the output's filename
* @return - EXIT_SUCCESS at the end
*/

#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPolyDataReader.h>
#include <vtkPointData.h>
#include <vtkPolyDataNormals.h>
#include <vtkPolyDataWriter.h>
#include <vtkAppendPolyData.h>
#include <vtkCleanPolyData.h>

#include "/work2/vt-system-install/include/vampirtrace/vt_user.h"
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#include <vtkRectilinearGrid.h>
#include <vtkRectilinearGridReader.h>

#include <vtkContourFilter.h>
#include <vtkPoints.h>

/**
 * This struct contains the id of the thread, the filename prefix of the 
 * vtk files, the number of total threads, and vtk poly data that has been
 * outputted by vtkContourFilter. This will be useful for determining which 
 * vtk file goes with which particular thread.
*/
typedef struct Param_Function
{
    const char *VTKinput;
    int threadId;
    vtkPolyData* vtkPiece;
} params;

/**
 * This function takes in the appropriate vtk Rectilinear file and thread. The
 * thread reads the file and applies vtkContourFilter to the data. vtk polydata
 * is outputted, and are then sent to the parent thread. 
*/
void* thread_function(void* ptr)
{
    /* This is a struct variable that is useful for determining which vtk 
     * rectilinear file goes with which pthreads, as well as in the 
       conglomeration of the pieces of vtk data */
    params* NewPtr;
    NewPtr = (params*) ptr;

    VT_ON();
    VT_USER_START("Region 1");

    int NumOfCharPD = strlen(NewPtr->VTKinput) + 5 + 1;
    int original = NumOfCharPD;

    // figure out how many characters are in the temporary file name, 
    if(NewPtr->threadId > 9)
        NumOfCharPD += (int) log10(NewPtr->threadId);

    char buf[(int) NumOfCharPD - original + 1];

    sprintf(buf, "%d", NewPtr->threadId);

    const char* suffix = ".vtk";

    const char* prefix = NewPtr->VTKinput;

    char prefix_suffix[NumOfCharPD];

    strcpy(prefix_suffix, prefix);

    strcat(prefix_suffix, buf);

    strcat(prefix_suffix, suffix);

    VT_USER_END("Region 1");
    VT_OFF();

    vtkRectilinearGridReader *reader = vtkRectilinearGridReader::New();

    VT_ON();
    VT_USER_START("Region 2");
    
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

    // Save the vtk poly data as a member of the struct
    NewPtr->vtkPiece = triangleCellNormals->GetOutput();

    pointdata->Delete();

    VT_USER_END("Region 4");
    VT_OFF();
}

/**
 * The main function calls the thread function. The vtkpolydata outputted 
 * from these functions are collected and appended together to form one
 * output vtkpolydata file.
 */
int main(int argc, char *argv[])
{
    struct timespec t0,t1;

    clock_gettime(CLOCK_REALTIME,&t0);

    /* Number of threads */
    int size = atoi(argv[1]);

    /* Array with elements of type params (the structure defined above) */
    params thread_data_array[size];

	pthread_t threads[size];

	for (int f = 0; f < size; f++) {      
        //creating threads
        // The file extension to look for is .f.vtk
        std::string fileprefix = argv[3];

        thread_data_array[f].VTKinput = fileprefix.c_str();
        thread_data_array[f].threadId = f;
		pthread_create(&threads[f], NULL, thread_function, (void*)&thread_data_array[f]);
	}

    VT_ON();
    VT_USER_START("Region 5");

    // Joining threads to make sure all threads are terminated
	for (int j = 0; j < size; j++)
    {
		pthread_join(threads[j], NULL);
    }

    /* to append each piece into 1 big vtk file */
    vtkAppendPolyData *appendWriter = vtkAppendPolyData::New();

    // Go through the array of structs, get the vtk data, and append 
    // the vtk data
    for(int k = 0; k < size; k++)
    {
        vtkPolyData *inputNum = vtkPolyData::New();

        inputNum->ShallowCopy(thread_data_array[k].vtkPiece);

        appendWriter->AddInput(thread_data_array[k].vtkPiece);

        appendWriter->Update();

        inputNum->Delete();
    }

    // Output vtkpolydata file
    vtkPolyDataWriter *pWriter = vtkPolyDataWriter::New();
    pWriter->SetFileName(argv[2]);

    pWriter->SetInput(appendWriter->GetOutput());

    pWriter->Write();

    VT_USER_END("Region 5");
    VT_OFF();

    clock_gettime(CLOCK_REALTIME,&t1);

    double dt = (double) (t1.tv_sec - t0.tv_sec) + (double) (t1.tv_nsec - t0.tv_nsec) / 1.0e9;

    printf("The measured time elapsed to be: %f\n", dt);

	return EXIT_SUCCESS;
}
