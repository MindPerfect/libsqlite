
file(GLOB tgtSrcs *.cc)
#-file(GLOB tgtHdrs *.h *.hh *.hpp)
set(tgt sqlite_test)

add_executable(${tgt})
add_test(NAME ${tgt} COMMAND ${tgt})

target_sources(${tgt} PRIVATE ${tgtSrcs})
target_include_directories(${tgt}
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${PrjSrc}
)
target_link_libraries(${tgt} PUBLIC lib_static GTest::gtest)
