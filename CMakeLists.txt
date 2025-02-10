cmake_minimum_required(VERSION 3.15)

set(EXTERNAL_FOLDER "External")
set(LIBRARIES_FOLDER "Libraries")
set(SAMPLES_FOLDER "Samples")

# Get the directory name where CMakeLists.txt resides
get_filename_component(PROJECT_NAME ${CMAKE_SOURCE_DIR} NAME)
project(${PROJECT_NAME})

# Enable folders in Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Move CMake auto-generated targets (like ALL_BUILD, ZERO_CHECK) into "CMake Targets"
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake Targets")

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# List of external libraries
set(EXTERNAL_LIBRARIES "")

# Collect all libraries
set(ALL_LIBRARIES "")

# Macro to parse YAML dependencies from uses.cfg
macro(parse_yaml_dependencies YAML_FILE OUT_EXTERNAL OUT_LIBRARIES OUT_GLOBAL)
    if (EXISTS ${YAML_FILE})
        file(READ ${YAML_FILE} YAML_CONTENTS)
        string(REPLACE "\n" ";" YAML_LINES ${YAML_CONTENTS})
        
        set(CURRENT_SECTION "")
        set(${OUT_EXTERNAL} "")
        set(${OUT_LIBRARIES} "")
        set(${OUT_GLOBAL} "")

        foreach(LINE ${YAML_LINES})
            string(STRIP "${LINE}" LINE)
            
            if (LINE MATCHES "^- External$")
                set(CURRENT_SECTION "EXTERNAL")
            elseif (LINE MATCHES "^- Libraries$")
                set(CURRENT_SECTION "LIBRARIES")
            elseif (LINE MATCHES "^- Globals$")
                set(CURRENT_SECTION "GLOBAL")
            elseif (CURRENT_SECTION STREQUAL "EXTERNAL" AND LINE MATCHES "^- (.+)")
                list(APPEND ${OUT_EXTERNAL} "${CMAKE_MATCH_1}")
            elseif (CURRENT_SECTION STREQUAL "LIBRARIES" AND LINE MATCHES "^- (.+)")
                list(APPEND ${OUT_LIBRARIES} "${CMAKE_MATCH_1}")
            elseif (CURRENT_SECTION STREQUAL "GLOBAL" AND LINE MATCHES "^- (.+)")
                list(APPEND ${OUT_GLOBAL} "${CMAKE_MATCH_1}")
            endif()
        endforeach()
    endif()
endmacro()

# Macro to link dependencies from uses.cfg
macro(link_dependencies TARGET_NAME CONFIG_FILE)
    # Parse `uses.cfg` for dependencies
    set(REQUIRED_EXTERNAL "")
    set(REQUIRED_LIBRARIES "")
    set(REQUIRED_GLOBAL "")
    parse_yaml_dependencies(${CONFIG_FILE} REQUIRED_EXTERNAL REQUIRED_LIBRARIES REQUIRED_GLOBAL)

    # Link External dependencies
    foreach(DEP ${REQUIRED_EXTERNAL})
        if (TARGET ${DEP})
            target_link_libraries(${TARGET_NAME} PRIVATE ${DEP})
        else()
            message(WARNING "External dependency '${DEP}' not found for target ${TARGET_NAME}")
        endif()
    endforeach()

    # Link Local Libraries
    foreach(DEP ${REQUIRED_LIBRARIES})
        if (TARGET ${DEP})
            target_link_libraries(${TARGET_NAME} PRIVATE ${DEP})
        else()
            message(WARNING "Library '${DEP}' not found for target ${TARGET_NAME}")
        endif()
    endforeach()

    # Find and link Global dependencies
    foreach(DEP ${REQUIRED_GLOBAL})
        find_package(${DEP} REQUIRED)
        if (${DEP}_FOUND)
            target_link_libraries(${TARGET_NAME} PRIVATE ${DEP})
        else()
            message(WARNING "Global dependency '${DEP}' could not be found for target ${TARGET_NAME}!")
        endif()
    endforeach()
endmacro()

# Macro to add a library with `uses.cfg`
macro(add_library_with_dependencies LIB_NAME LIB_PATH FOLDER_NAME)
    add_library(${LIB_NAME})

    # Add all .cpp files in the src directory
    file(GLOB LIB_SOURCES ${LIB_PATH}/src/*.cpp)
    target_sources(${LIB_NAME} PRIVATE ${LIB_SOURCES})

    # Include the headers
    target_include_directories(${LIB_NAME} PUBLIC ${LIB_PATH}/include)

    # Link dependencies (uses.cfg)
    link_dependencies(${LIB_NAME} "${LIB_PATH}/uses.cfg")

    # Set Visual Studio solution folder
    set_target_properties(${LIB_NAME} PROPERTIES FOLDER ${FOLDER_NAME})

    # Store the library name in the global list
    list(APPEND ALL_LIBRARIES ${LIB_NAME})
endmacro()

# Macro to add a sample executable with `uses.cfg`
macro(add_sample SAMPLE_NAME SAMPLE_PATH)
    add_executable(${SAMPLE_NAME})

    # Add all .cpp files in the src directory
    file(GLOB SAMPLE_SOURCES ${SAMPLE_PATH}/src/*.cpp)
    target_sources(${SAMPLE_NAME} PRIVATE ${SAMPLE_SOURCES})

    # Include the headers
    target_include_directories(${SAMPLE_NAME} PRIVATE ${SAMPLE_PATH}/include)

    # Link dependencies (uses.cfg)
    link_dependencies(${SAMPLE_NAME} "${SAMPLE_PATH}/uses.cfg")

    # Set Visual Studio solution folder
    set_target_properties(${SAMPLE_NAME} PROPERTIES FOLDER "Samples")
endmacro()

# Scan and add all external dependencies (External/*)
file(GLOB EXTERNAL_DIRS RELATIVE ${CMAKE_SOURCE_DIR}/External ${CMAKE_SOURCE_DIR}/External/*)
foreach(EXT_LIB ${EXTERNAL_DIRS})
    if (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/External/${EXT_LIB})
        add_library_with_dependencies(${EXT_LIB} ${CMAKE_SOURCE_DIR}/External/${EXT_LIB} "External")
    endif()
endforeach()

# Scan and add all libraries in Libraries/
file(GLOB LIBRARY_DIRS RELATIVE ${CMAKE_SOURCE_DIR}/Libraries ${CMAKE_SOURCE_DIR}/Libraries/*)
foreach(LIB ${LIBRARY_DIRS})
    if (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/Libraries/${LIB})
        add_library_with_dependencies(${LIB} ${CMAKE_SOURCE_DIR}/Libraries/${LIB} "Libraries")
    endif()
endforeach()

# Scan and add all samples (Samples/*)
file(GLOB SAMPLE_DIRS RELATIVE ${CMAKE_SOURCE_DIR}/Samples ${CMAKE_SOURCE_DIR}/Samples/*)
foreach(SAMPLE ${SAMPLE_DIRS})
    if (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/Samples/${SAMPLE})
        add_sample(${SAMPLE} ${CMAKE_SOURCE_DIR}/Samples/${SAMPLE})
    endif()
endforeach()