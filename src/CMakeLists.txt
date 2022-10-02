add_subdirectory(aqmapgen)

add_executable(animtool animtool.cpp)
target_link_libraries(animtool deps)

add_executable(aqnavmapgen aqnavmapgen.cpp)
target_link_libraries(aqnavmapgen deps)

add_executable(aqsc aqsc.cpp)
target_link_libraries(aqsc deps)

add_executable (palgen palgen.cpp)
target_link_libraries(palgen deps)

add_executable (pngrim pngrim.cpp)
target_link_libraries(pngrim deps)
