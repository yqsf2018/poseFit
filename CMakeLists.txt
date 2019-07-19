# cmake needs this line
cmake_minimum_required(VERSION 3.1)

# Enable C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
# set(GDB_FLAG "-g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GDB_FLAG}")

# Define project name
PROJECT(fitPose)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# Find OpenCV, you may need to set OpenCV_DIR variable
# to the absolute path to the directory containing OpenCVConfig.cmake file
# via the command line or GUI
find_package(OpenCV REQUIRED)

# If the package has been found, several variables will
# be set, you can find the full list with descriptions
# in the OpenCVConfig.cmake file.
# Print some message showing some of them
message(STATUS "OpenCV library status:")
message(STATUS "    config: ${OpenCV_DIR}")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

include_directories( ${OpenCV_INCLUDE_DIRS} )
add_library(curlAgent curlExt)
add_library(dcCtrl dcCtrl)
add_library(objBoxFilter obj_box_filter)
add_library(picProc postProcPic)
add_library(parseArg argParser)
add_library(frmFeeder frmFeeder)
add_library(appInfo appInfo)
add_library(bodyDetThrd bodyDetThread)
add_library(faceDetect faceDetThread)

# INCLUDE_DIRECTORIES( /usr/local/include/opencv4)
# LINK_DIRECTORIES( /usr/local/lib )
find_library(CURLEXT_LIB1 curl PATHS /usr/lib/x86_64-linux-gnu)
find_library(CURLEXT_LIB2 json-c PATHS /usr/lib/x86_64-linux-gnu)
find_library(THREAD_LIB pthread PATHS /lib/i386-linux-gnu)
find_library(OD_LIB1 X11 PATHS /usr/lib/x86_64-linux-gnu)

# include(./dlib/dlib/cmake)

MACRO(add_example name)
  ADD_EXECUTABLE(${name} ${name}.cpp)
  TARGET_LINK_LIBRARIES(${name} ${OpenCV_LIBS} opencv_tracking)
ENDMACRO()

# Declare the executable target built from your sources
#add_example(face_detection_opencv_dnn)
#add_example(multitracker)
#add_example(object_detection)
#add_example(faceCapture)
add_example(bDetTest)
#add_example(bodyDetect)
#add_example(OpenPoseVideo)
#add_example(multi-person-openpose)
#add_example(OpenPoseImage)

#target_include_directories (
#    object_detection PUBLIC
#        /usr/include 
#        /usr/include/x86_64-linux-gnu
#    )

target_link_libraries(curlAgent ${CURLEXT_LIB1} ${CURLEXT_LIB2})
target_link_libraries(dcCtrl curlAgent appInfo)
target_link_libraries(picProc ${OpenCV_LIBS})
target_link_libraries(bodyDetThrd ${THREAD_LIB} ${OpenCV_LIBS} dcCtrl objBoxFilter)
# target_link_libraries(faceDetect picProc ${THREAD_LIB} ${OpenCV_LIBS} appInfo)
# target_link_libraries(bodyDetect ${CURLEXT_LIB1} ${CURLEXT_LIB2} ${THREAD_LIB} ${OD_LIB1} picProc bodyDetThrd)
# target_link_libraries(object_detection ${CURLEXT_LIB1} ${CURLEXT_LIB2} ${THREAD_LIB} ${OD_LIB1} picProc bodyDetThrd)
# target_link_libraries(faceCapture ${THREAD_LIB} ${OD_LIB1} parseArg frmFeeder appInfo faceDetect)
target_link_libraries( bDetTest ${THREAD_LIB} ${OpenCV_LIBS} ${OD_LIB1} parseArg frmFeeder picProc bodyDetThrd)

add_example(curlExtTest)
target_include_directories (
    curlExtTest PUBLIC
        /usr/include 
        /usr/include/x86_64-linux-gnu
    )
target_link_libraries(curlExtTest ${CURLEXT_LIB1} ${CURLEXT_LIB2} ${THREAD_LIB} curlAgent)

