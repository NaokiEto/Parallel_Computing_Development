cmake_minimum_required(VERSION 2.8)

PROJECT(ApplyingVtkContourFilter)

set(VAMPIRTRACE_ROOT $ENV{ROOT}/work2/vt-system-install)
set(VAMPIRTRACE_LIBRARIES ${VAMPIRTRACE_ROOT}/lib/libotfaux.so
                          ${VAMPIRTRACE_ROOT}/lib/libvt-mpi.so)

include_directories(${VAMPIRTRACE_ROOT}/include)
add_definitions( -DVTRACE )

set(VTK_DIR $ENV{ROOT}/work2/VTK5.10.1-install/lib/vtk-5.10)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

add_executable(ApplyingVtkContourFilter ApplyingVtkContourFilter.cxx)

SET(CMAKE_C_COMPILER mpicc)

SET(CMAKE_CXX_COMPILER mpicxx)

target_link_libraries(ApplyingVtkContourFilter ${VAMPIRTRACE_LIBRARIES})

target_link_libraries(ApplyingVtkContourFilter mpi)

if(VTK_LIBRARIES)
  target_link_libraries(ApplyingVtkContourFilter ${VTK_LIBRARIES})
else()
  target_link_libraries(ApplyingVtkContourFilter vtkHybrid)
endif()
