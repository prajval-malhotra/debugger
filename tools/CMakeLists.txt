# create a target called sdb
add_executable(sdb sdb.cpp)
# specify dependency on libsdb - handles adding public include dir 
#   to include paths for buidling
target_link_libraries(sdb PRIVATE sdb::libsdb PkgConfig::readline)


include(GNUInstallDirs)
install(
    TARGETS sdb
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)