# Add httplib as a header-only library
add_library(httplib INTERFACE)
target_include_directories(httplib INTERFACE httplib)
