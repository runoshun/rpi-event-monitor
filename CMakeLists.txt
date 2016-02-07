cmake_minimum_required (VERSION 2.8)
project (rpi-event-monitor)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

set (INCLUDE_DIRS "")
foreach (_headerFile ${HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)

find_package(PkgConfig)

pkg_check_modules (CEC libcec)
pkg_check_modules (GLIB2 glib-2.0)
pkg_check_modules (GIO2 gio-2.0)

include_directories(${INCLUDE_DIRS} ${CEC_INCLUDE_DIRS} ${GLIB2_INCLUDE_DIRS} ${GIO2_INCLUDE_DIRS})
add_executable(rpi-event-monitor ${SOURCES})

# set fixed cec and p8-platform library due to cmake bug?
#target_link_libraries(rpi-event-monitor ${CEC_LIBRARIES} ${GLIB2_LIBRARIES} ${GIO2_LIBRARIES} dl)
target_link_libraries(rpi-event-monitor cec p8-platform ${GLIB2_LIBRARIES} ${GIO2_LIBRARIES} dl)

