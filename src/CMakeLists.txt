file(GLOB CPP "../cpp/*.cpp")
add_executable(
  merge-vertices
    main.cpp
    ${CPP}
)

set_target_properties(
  merge-vertices
    PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY "../lib/"
      LIBRARY_OUTPUT_DIRECTORY "../lib/"
      RUNTIME_OUTPUT_DIRECTORY "../"
)

target_link_libraries(
  merge-vertices
#    ${Boost_LIBRARIES}
    filePLY
    rply
)

add_executable(
  epsilon-finder
    find-eps.cpp
    ${CPP}
)

set_target_properties(
  epsilon-finder
    PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY "../lib/"
      LIBRARY_OUTPUT_DIRECTORY "../lib/"
      RUNTIME_OUTPUT_DIRECTORY "../"
)


target_link_libraries(
  epsilon-finder
#    ${Boost_LIBRARIES}
    filePLY
    rply
)
