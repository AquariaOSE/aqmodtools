add_subdirectory(zlib)

include_directories(zlib)
include_directories(libpng)
add_subdirectory(libpng)

add_library(imagepng ImagePNG.cpp ImagePNG.h)
target_link_libraries(imagepng png)

add_library(tinyxml2 tinyxml2.cpp tinyxml2.h)

