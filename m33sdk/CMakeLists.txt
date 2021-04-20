project(m33sdk VERSION 1.0)

add_library(m33sdk INTERFACE)

target_link_directories(m33sdk INTERFACE lib)
target_include_directories(m33sdk INTERFACE include)