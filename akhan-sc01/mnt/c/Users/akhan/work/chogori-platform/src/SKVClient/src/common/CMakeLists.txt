file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(skv_common  STATIC ${HEADERS} ${SOURCES})

set_target_properties(common PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

target_link_libraries (skv_common PRIVATE Boost::thread Threads::Threads fmt::fmt)

# export the library in the common skvTargets
install(TARGETS skv_common EXPORT skvTargets DESTINATION lib/skv)
install(FILES ${HEADERS} DESTINATION include/skv/common)
# export the cmake config in the build tree for any users who want to use this project from source
export(TARGETS skv_common NAMESPACE skv:: FILE common-config.cmake)
