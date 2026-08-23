cmake_minimum_required(VERSION 3.15)
project(SevenZipContextMenu LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX)

if (MSVC)
    add_compile_options(
        /O2         # Maximize speed
        /Oi         # Enable intrinsic functions
        /Ot         # Favor fast code
        /GL         # Whole program optimization
        /GF         # Eliminate duplicate strings
        /Gy         # Function-level linking
        /MP         # Multi-processor compilation
        /W4         # High warning level
    )
    add_link_options(
        /LTCG       # Link-time code generation
        /OPT:REF    # Eliminate unreferenced data/functions
        /OPT:ICF    # Fold identical COMDATs
    )
endif()

set(DLL_SOURCES
    src/dllmain.cpp
    src/ClassFactory.cpp
    src/SevenZipCommand.cpp
    src/SevenZipHelper.cpp
    src/resource.rc
    src/SevenZipContextMenu.def
)

set(DLL_HEADERS
    src/Guid.h
    src/ClassFactory.h
    src/SevenZipCommand.h
    src/SevenZipHelper.h
    src/resource.h
)

# Build the COM DLL
add_library(SevenZipContextMenu SHARED ${DLL_SOURCES} ${DLL_HEADERS})

target_link_libraries(SevenZipContextMenu PRIVATE
    shlwapi
    shell32
    ole32
    advapi32
    user32
)

set_target_properties(SevenZipContextMenu PROPERTIES
    OUTPUT_NAME "SevenZipContextMenu"
    PREFIX ""
)

# Build the host Win32 executable
add_executable(SevenZipContextMenuExe WIN32
    src/main.cpp
    src/SevenZipHelper.cpp
    src/resource.rc
)

target_link_libraries(SevenZipContextMenuExe PRIVATE
    shlwapi
    shell32
    ole32
    advapi32
    user32
)

set_target_properties(SevenZipContextMenuExe PROPERTIES
    OUTPUT_NAME "SevenZipContextMenu"
)
