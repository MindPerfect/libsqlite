
# Library sources
set(LibSrc sqlite3.c Sqlite.cc SqliteUtils.cc)
set(LibHdr sqlite3.h sqlite3ext.h Sqlite.hh SqliteUtils.hh)

# Executable (shell) sources
set(ExeSrc shell.c sqlite3.c)


add_definitions(-DSQLITE_ENABLE_RTREE)
add_definitions(-DSQLITE_ENABLE_FTS4)
add_definitions(-DSQLITE_ENABLE_FTS5)
add_definitions(-DSQLITE_ENABLE_JSON1)
add_definitions(-DSQLITE_ENABLE_RBU)
add_definitions(-DSQLITE_ENABLE_STAT4)

include_directories(${PrjSrc})

# Static Sqlite Library
add_library(lib_static STATIC ${LibSrc})
set_target_properties(lib_static PROPERTIES LINKER_LANGUAGE CXX OUTPUT_NAME sqlite3)
target_link_libraries(lib_static absl::base absl::log absl::strings)

# Dynamic Sqlite Library
add_library(lib_shared SHARED ${LibSrc})
set_target_properties(lib_shared PROPERTIES LINKER_LANGUAGE CXX OUTPUT_NAME sqlite3dl)
set_property(TARGET lib_shared PROPERTY POSITION_INDEPENDENT_CODE 1)
target_link_libraries(lib_shared absl::base absl::log absl::strings)

# Sqlite shell
add_executable(exe ${ExeSrc})
set_target_properties(exe PROPERTIES OUTPUT_NAME sqlite3)


if(WIN32)
  #Todo: Exe installation doesn't work 
  add_custom_command(TARGET exe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:exe> ${CMAKE_BINARY_DIR}/sqlite3.exe
    DEPENDS exe
  )
  #install(FILES sqlite3.exe DESTINATION bin) #$<TARGET_FILE:exe>
  install(TARGETS exe RUNTIME DESTINATION bin)
  install(FILES ${CMAKE_BINARY_DIR}/sqlite3dl.dll DESTINATION lib)
else()
  include(FindThreads)
  target_link_libraries(exe ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} m)
  install(TARGETS exe RUNTIME DESTINATION bin)
endif()

install(FILES ${libHdr} DESTINATION include)
install(TARGETS lib_static LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(TARGETS lib_shared LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#[[

#-file(GLOB libSrcs *.c *.cc)
#-file(GLOB libHdrs *.h *.hh *.hpp)

# Target Library name
set(tgt sqlite)


add_library(${tgt} STATIC)
# https://cmake.org/cmake/help/latest/command/target_sources.html
# https://stackoverflow.com/questions/10487256/cmake-how-to-properly-copy-static-librarys-header-file-into-usr-include
target_sources(${tgt}
    PRIVATE ${libSrcs}
    PUBLIC FILE_SET HEADERS FILES ${libHdrs}
)
set_target_properties(${tgt} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${tgt} absl::base absl::debugging absl::log absl::strings)


install(
  TARGETS sqlite
  EXPORT ${TARGETS_EXPORT_NAME}
  ARCHIVE  DESTINATION ${INSTALL_LIBDIR} COMPONENT lib
  FILE_SET HEADERS DESTINATION ${INSTALL_INCDIR}
)
#]]