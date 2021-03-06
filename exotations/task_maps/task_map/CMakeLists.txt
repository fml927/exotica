cmake_minimum_required(VERSION 2.8.3)
project(task_map)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
set(CMAKE_EXE_LINKER_FLAGS -Wl,--no-as-needed)
set(CMAKE_SHARED_LINKER_FLAGS -Wl,--no-as-needed)
set(CMAKE_MODULE_LINKER_FLAGS -Wl,--no-as-needed)

add_definitions(-DEXOTICA_DEBUG_MODE)

find_package(catkin REQUIRED COMPONENTS
  exotica
  geometry_msgs
)

catkin_package(
  INCLUDE_DIRS include
  LIBRARIES ${PROJECT_NAME}
  CATKIN_DEPENDS exotica geometry_msgs
  DEPENDS #system_lib
)

###########
## Build ##
###########

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME} src/kinematic_maps/CoM.cpp
                            src/kinematic_maps/IMesh.cpp
                            src/kinematic_maps/DMesh.cpp
                            src/kinematic_maps/EffPosition.cpp
                            src/kinematic_maps/JointLimit.cpp
                            src/kinematic_maps/SweepFlux.cpp
                            src/kinematic_maps/Distance.cpp
                            src/kinematic_maps/Orientation.cpp
                            src/kinematic_maps/CollisionAvoidance.cpp
                            src/kinematic_maps/GJKCollisionAvoidance.cpp
                            src/generic/Identity.cpp
                            src/kinematic_maps/JointSpaceSampling.cpp
           )


target_link_libraries(${PROJECT_NAME} ${catkin_LIBRARIES})

## Install
install(TARGETS ${PROJECT_NAME}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})
install(DIRECTORY include/ DESTINATION include)
