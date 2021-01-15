# library name
set(LIBNAME "milkCOREMODarith")
set(SRCNAME "COREMOD_arith")


set(SOURCEFILES
	${SRCNAME}.c
	set_pixel.c
	image_crop.c
	image_merge3D.c
	image_total.c
	image_stats.c
	image_dxdy.c
	imfunctions.c
	mathfuncs.c
	image_arith__im__im.c
	image_arith__im_im__im.c
	image_arith__Cim_Cim__Cim.c
	image_arith__im_f__im.c
	image_arith__im_f_f__im.c
	execute_arith.c
)

set(INCLUDEFILES
	${SRCNAME}.h
	set_pixel.h
	image_crop.h
	image_merge3D.h
	image_total.h
	image_stats.h
	image_dxdy.h
	imfunctions.h
	mathfuncs.h
	image_arith__im__im.h
	image_arith__im_im__im.h
	image_arith__Cim_Cim__Cim.h
	image_arith__im_f__im.h
	image_arith__im_f_f__im.h
	execute_arith.h
)


# DEFAULT SETTINGS
# Do not change unless needed
# =====================================================================

project(lib_${LIBNAME}_project)

include_directories ("${PROJECT_SOURCE_DIR}/src")
include_directories ("${PROJECT_SOURCE_DIR}/..")


# Library can be compiled from multiple source files
# Convention: the main souce file is named <libname>.c
#
add_library(${LIBNAME} SHARED ${SOURCEFILES})

find_package(PkgConfig REQUIRED)
pkg_check_modules(CFITSIO cfitsio)
if(${CFITSIO_FOUND})
  link_directories(${CFITSIO_LIBRARY_DIRS})
  target_compile_definitions(${LIBNAME} PRIVATE USE_CFITSIO=1)
  target_include_directories(${LIBNAME} PRIVATE ${CFITSIO_INCLUDE_DIRS})
endif()

target_link_libraries(${LIBNAME} PUBLIC m ${CFITSIO_LIBRARIES})

install(TARGETS ${LIBNAME} DESTINATION lib)
install(FILES ${INCLUDEFILES} DESTINATION include/${SRCNAME})