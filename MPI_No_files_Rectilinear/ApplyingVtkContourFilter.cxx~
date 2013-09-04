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
*        rectilinear file to the appropriate process, passes the data
*        through vtkContourFilter, which outputs the resulting VTK polydata, 
*        and then conglomerates the data into 1 vtk file in the parent process.
*        The time is outputted into a file.
* @param[in] number of processes - number of processes for MPI (look at README 
             for more information)
* @param[in] argv[1] - the output's filename
* @param[in] argv[2] - the prefix of the files (i.e. 27noise.vtk.)
* @param[out] pWriter - vtkPolyData file with the output's filename
* @return - EXIT_SUCCESS at the end
*/

#include <vtkSmartPointer.h>
#include <vtkPolyDataReader.h>
#include <vtkPointData.h>
#include <vtkPolyDataNormals.h>
#include <vtkPolyDataWriter.h>
#include <vtkAppendPolyData.h>
#include <vtkCleanPolyData.h>
#include <vtkMPIController.h>
#include <vtkPolyData.h>

#include <mpi.h>//"/home/users/neto/mpich2-1.4.1p1-install/include/mpi.h"
#include <stdio.h>

#include <vtkRectilinearGrid.h>
#include <vtkRectilinearGridReader.h>
#include <vtkContourFilter.h>
#include <vtkPoints.h>

#include <time.h>

void process(int procRank, int procSize, vtkMPIController* procController, const char* fp)
{
    /* The vtk file extension we want to search for */

    int NumOfCharPD = strlen(fp) + 5 + 1;
    int original = NumOfCharPD;

    // figure out how many characters are in the temporary file name, 
    if(procRank - 1 > 9)
        NumOfCharPD += (int) log10(procRank - 1);

    char buf[(int) NumOfCharPD - original + 1];
 
    sprintf(buf, "%d", procRank-1);

    const char* suffix = ".vtk";

    const char* prefix = fp;

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

    // send the vtkPolyData to the parent process
    procController->Send(triangleCellNormals->GetOutput(), 0, 101);

    triangleCellNormals->Delete();
    pointdata->Delete();
}

/**
 * This program takes in vtkRectilinear files and assigns the appropriate
 * file (which is numbered) with the appropriate child processor. In each 
 * child processor, vtkContourFilter is applied to the piece of data, and 
 * a vtk polydata is outputted and sent to the parent processor. During this
 * time, the parent processor is waiting to receive this data from the child
 * processors. The parent processor conglomerates the data, and an output vtk 
 * polydata file is outputted.
 */
int main(int argc, char *argv[])
{
    vtkMPIController* controller = vtkMPIController::New();

    double t1 = MPI_Wtime();

    // Initializing MPI
    controller->Initialize(&argc, &argv);

    /* Figure out total amount of processors */
    int rank = controller->GetLocalProcessId();

    /* Figure out the rank of this processor */
    int size = controller->GetNumberOfProcesses();

    // If not parent process, do the vtkContourFilter implementation
    if (rank != 0)
    {
        const char* prefix = argv[2];
        process(rank, size, controller, prefix);
    }

    // Parent
    else
    {   
        // to append each piece into 1 big vtk file
        vtkAppendPolyData *appendWriter = vtkAppendPolyData::New();

        // go through the child processes, and append
        for(int k = 1; k < size; k++)
        {
            vtkPolyData* pd = vtkPolyData::New();
            
            controller->Receive(pd, k, 101);

            appendWriter->AddInput(pd);

            appendWriter->Update();

            pd->Delete();
        }

        vtkPolyDataWriter *pWriter = vtkPolyDataWriter::New();
        pWriter->SetFileName(argv[1]);

        pWriter->SetInput(appendWriter->GetOutput());

        // output vtk file
        pWriter->Write();

        double t2 = MPI_Wtime();

        double time = t2 - t1;

        printf("MPI_Wtime measured the time elapsed to be: %f\n", time);

        FILE *out_file = fopen("../OutPutFile.txt", "a"); // write only 

        char strPD[6];

	snprintf(strPD, 7, "%f \n", time);

        fputs(strPD, out_file);
        fclose(out_file);
    }

    controller->Finalize(); 
    controller->Delete();

    return EXIT_SUCCESS;
}
