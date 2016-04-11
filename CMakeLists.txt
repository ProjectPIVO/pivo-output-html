# Copyright (C) 2016 Martin Ubl <http://pivo.kennny.cz>
#
# This file is part of PIVO html output module.
#
# PIVO html output module is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, either version 3 of
# the Licence, or (at your option) any later version.
#
# PIVO html output module is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with PIVO html output module. If not,
# see <http://www.gnu.org/licenses/>.

# Define macro for selecting "all subdirectories"
MACRO(SUBDIRLIST result curdir)
    FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
    SET(dirlist "")
    FOREACH(child ${children})
        IF(IS_DIRECTORY ${curdir}/${child})
            LIST(APPEND dirlist ${child})
        ENDIF()
    ENDFOREACH()
    SET(${result} ${dirlist})
ENDMACRO()

# Retrieve list of all subdirectories
SUBDIRLIST(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR})

# Prepare file list (empty for now)
SET(modulefiles )

# Retrieve core include directories to be included
GET_PROPERTY(core_includes GLOBAL PROPERTY core_include_dirs)

# Go through all subdirectories
FOREACH(subdir ${SUBDIRS})
    # All of them should also serve as include directories
    INCLUDE_DIRECTORIES(${INCLUDE_DIRECTORIES}
        ${core_includes}
        ${subdir}
    )

    # Search for all source and header files, and append them
    FILE(GLOB tmp_src
        ${subdir}/*.h
        ${subdir}/*.cpp
        ${subdir}/*.c
    )

    # Create filter (MS Visual Studio) for every subdirectory
    SOURCE_GROUP(${subdir} FILES ${tmp_src})

    # Append current source list to all file list
    SET(modulefiles
        ${modulefiles}
        ${tmp_src}
    )

    # Report this subdirectory
    MESSAGE(STATUS "Added source directory " ${subdir})
ENDFOREACH()

# add output shared library
ADD_LIBRARY(pivo-output-html SHARED ${modulefiles})

# copy HtmlTemplates directory alongside built library
GET_FILENAME_COMPONENT(templatepath "HtmlTemplates" REALPATH)
ADD_CUSTOM_COMMAND(TARGET pivo-output-html PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${templatepath} $<TARGET_FILE_DIR:pivo-output-html>/HtmlTemplates)
