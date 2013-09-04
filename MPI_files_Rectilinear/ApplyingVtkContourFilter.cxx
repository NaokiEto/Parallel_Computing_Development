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
*        through vtkContourFilter, which outputs the resulting VTK polydata.
*        The data is written in temporary files, which are sent to the parent
*        process (rank 0), which then conglomerates the data in the temporary
*        files into 1 vtk file. The time is outputted into a file.
* @param[in] number of processes - number of processes for MPI (look at README 
             for more information)
* @param[in] argv[1] - the output's filename
* @param[in] argv[2] - the prefix of the files (i.e. 27noise.vtk.)
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

#include <vtkContourFilter.h>
#include <vtkPoints.h>

#include <mpi.h>
#include <stdio.h>

#include <vtkRectilinearGrid.h>
#include <vtkRectilinearGridReader.h>

#include <time.h>

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
    /* Number of processes */
    static int size;

    /* Rank of process */
    int rank;

    MPI_Status status;

    // Initializing MPI
    MPI_Init (&argc, &argv);

    double t1 = MPI_Wtime();

    // Figure out total amount of processors
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Figure out the rank of this processor
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int NumOfCharPD;

    char strPD[NumOfCharPD];

    // If not master process, do the vtkMarchingCubes implementation
    if (rank >= 1)
    {
        /* This is for the temporary file names created. It contains the number 
           of characters in the temporary filename "ShrimpChowFun#.vtk" */
        NumOfCharPD = 18 + log10(rank);

        /* The vtk file extension we want to search for */

        int NumOfCharIN = strlen(argv[2]) + 5 + 1;
        int original = NumOfCharIN;

        // figure out how many characters are in the temporary file name, 
        if(rank - 1 > 9)
            NumOfCharIN += (int) log10(rank - 1);

        char buf[(int) NumOfCharIN - original + 1];
     
        sprintf(buf, "%d", rank-1);

        const char* suffix = ".vtk";

        const char* prefix = argv[2];

        char prefix_suffix[NumOfCharIN];

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

        /* vtkPolyDataWriter for temporary file */
        vtkPolyDataWriter *PDwriter = vtkPolyDataWriter::New();

        sprintf(strPD, "ShrimpChowFun%d.vtk", rank);

        PDwriter->SetFileName(strPD);

	    PDwriter->SetInput(triangleCellNormals->GetOutput());

        PDwriter->Write();

        // Send this temporary file name to the master process
        MPI_Send(strPD, NumOfCharPD, MPI_CHAR, 0, 1, MPI_COMM_WORLD);

        triangleCellNormals->Delete();
        pointdata->Delete();
    }

    // Master
    else
    {
        /* to append each piece into 1 big vtk file */
        vtkAppendPolyData *appendWriter = vtkAppendPolyData::New();

        // Receive all the temporary file names
        for(int i = 1; i < size; i++)
        {
            NumOfCharPD = log10(i) + 18;

            MPI_Recv(strPD, NumOfCharPD, MPI_CHAR, i, 1, MPI_COMM_WORLD, &status);
        }

        // Go through the temporary files and append the vtk data
        for(int k = 1; k < size; k++)
        {
            vtkPolyData *inputNum = vtkPolyData::New();
            vtkPolyDataReader *reader = vtkPolyDataReader::New();

            /* number of characters in file name */
            int NumOfCharPDMASTER = log10(k) + 18;

            char strPDMASTER[NumOfCharPDMASTER];

            sprintf(strPDMASTER, "ShrimpChowFun%d.vtk", k);
            reader->SetFileName(strPDMASTER);
            reader->Update();
            inputNum->ShallowCopy(reader->GetOutput());

            appendWriter->AddInput(reader->GetOutput());

            // remove temporary files
            remove(strPDMASTER);

            appendWriter->Update();

            reader->Delete();
            inputNum->Delete();
        }

        // output vtkpolydata file
        vtkPolyDataWriter *pWriter = vtkPolyDataWriter::New();
        pWriter->SetFileName(argv[1]);

        pWriter->SetInput(appendWriter->GetOutput());

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

    MPI_Finalize();    

    return EXIT_SUCCESS;
}
