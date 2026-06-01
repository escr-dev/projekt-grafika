add_executable (projekt_testowy "projekt_testowy.cpp" "projekt_testowy.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET projekt_testowy PROPERTY CXX_STANDARD 20)
endif()
