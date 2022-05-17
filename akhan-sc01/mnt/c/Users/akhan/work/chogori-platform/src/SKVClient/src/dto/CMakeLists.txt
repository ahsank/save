file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.cpp")

add_library(skv_dto STATIC ${HEADERS} ${SOURCES})

set_target_properties(skv_dto PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

target_link_libraries (skv_dto PRIVATE common)

# export the library in the common skvTargets
install(TARGETS skv_dto EXPORT skvTargets DESTINATION lib/skv)
install(FILES ${HEADERS} DESTINATION include/skv/dto)
# export the cmake config in the build tree for any users who want to use this project from source
export(TARGETS skv_dto NAMESPACE skv:: FILE dto-config.cmake)
