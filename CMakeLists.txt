cmake_minimum_required(VERSION 3.15)

set(EXTERNAL_FOLDER "External")
set(LIBRARIES_FOLDER "Libraries")
set(SAMPLES_FOLDER "Samples")

# Get project name dynamically
get_filename_component(PROJECT_NAME ${CMAKE_SOURCE_DIR} NAME)
project(${PROJECT_NAME})

# Enable folders in Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Move CMake auto-generated targets into "CMake Targets"
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake Targets")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect all available dependencies
set(ALL_LIBRARIES "")
set(EXTERNAL_LIBRARIES "")
set(GLOBAL_LIBRARIES "")

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
            
            if (LINE MATCHES "^- ${EXTERNAL_FOLDER}$")
                set(CURRENT_SECTION "EXTERNAL")
            elseif (LINE MATCHES "^- ${LIBRARIES_FOLDER}$")
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
    set_target_properties(${SAMPLE_NAME} PROPERTIES FOLDER "${SAMPLES_FOLDER}")
endmacro()

# **Scan and Add Only Referenced External Dependencies**
set(REFERENCED_EXTERNAL_LIBS "")

# Scan all samples and libraries to determine required external dependencies
file(GLOB SAMPLE_DIRS RELATIVE ${CMAKE_SOURCE_DIR}/${SAMPLES_FOLDER} ${CMAKE_SOURCE_DIR}/${SAMPLES_FOLDER}/*)
file(GLOB LIBRARY_DIRS RELATIVE ${CMAKE_SOURCE_DIR}/${LIBRARIES_FOLDER} ${CMAKE_SOURCE_DIR}/${LIBRARIES_FOLDER}/*)

foreach(DIR ${SAMPLE_DIRS} ${LIBRARY_DIRS})
    if (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/${SAMPLES_FOLDER}/${DIR})
        set(USES_CFG "${CMAKE_SOURCE_DIR}/${SAMPLES_FOLDER}/${DIR}/uses.cfg")
    elseif (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/${LIBRARIES_FOLDER}/${DIR})
        set(USES_CFG "${CMAKE_SOURCE_DIR}/${LIBRARIES_FOLDER}/${DIR}/uses.cfg")
    endif()

    if (EXISTS ${USES_CFG})
        set(REQUIRED_EXTERNAL "")
        set(REQUIRED_LIBRARIES "")
        set(REQUIRED_GLOBAL "")
        parse_yaml_dependencies(${USES_CFG} REQUIRED_EXTERNAL REQUIRED_LIBRARIES REQUIRED_GLOBAL)
        list(APPEND REFERENCED_EXTERNAL_LIBS ${REQUIRED_EXTERNAL})
    endif()
endforeach()

# **Add Only the Referenced External Dependencies**
foreach(EXT_LIB ${REFERENCED_EXTERNAL_LIBS})
    set(LIB_PATH "${CMAKE_SOURCE_DIR}/${EXTERNAL_FOLDER}/${EXT_LIB}")
    if (IS_DIRECTORY ${LIB_PATH})
        add_library_with_dependencies(${EXT_LIB} ${LIB_PATH} "${EXTERNAL_FOLDER}")
    endif()
endforeach()

# **Add Local Libraries**
foreach(LIB ${LIBRARY_DIRS})
    if (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/${LIBRARIES_FOLDER}/${LIB})
        add_library_with_dependencies(${LIB} ${CMAKE_SOURCE_DIR}/${LIBRARIES_FOLDER}/${LIB} "${LIBRARIES_FOLDER}")
    endif()
endforeach()

# **Add Samples**
foreach(SAMPLE ${SAMPLE_DIRS})
    if (IS_DIRECTORY ${CMAKE_SOURCE_DIR}/${SAMPLES_FOLDER}/${SAMPLE})
        add_sample(${SAMPLE} ${CMAKE_SOURCE_DIR}/${SAMPLES_FOLDER}/${SAMPLE})
    endif()
endforeach()
