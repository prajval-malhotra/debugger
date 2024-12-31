# specify target and source
add_library(libsdb process.cpp)
add_library(sdb::libsdb ALIAS libsdb)

# customize executable name (libsdb --> sdb)
set_target_properties(
    libsdb
    PROPERTIES OUTPUT_NAME sdb
)

# specify compiler version
target_compile_features(libsdb PUBLIC cxx_std_17)

# the following is required to make libsdb importable by other CMake projects

# list of include dirs for libsdba
target_include_directories(
    libsdb
    PUBLIC # any targets compiling against libsdb will inherit this
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/include
)

#  store info about lib target in a format that can be saved to disk to provide 
#   info about how to use the lib, where its installed 
include(GNUInstallDirs)  # contains vars specifying common installation dirs
install(TARGETS libsdb
    EXPORT sdb-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# install public headers
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# save export info to a file 
install(
    EXPORT sdb-targets
    FILE sdb-config.cmake
    NAMESPACE sdb::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sdb
)

# all this infor is installed to sdb-config.cmake
# exported targets should be under sdb namespace so users can link to sdb::libsdb