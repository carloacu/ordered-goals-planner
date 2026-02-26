cmake_minimum_required(VERSION 3.14)

project(ordered_goals_planner_bin)

include_directories(".")


if(NOT ORDEREDGOALSPLANNER_FOUND)
  find_package(ordered_goals_planner_lib)
endif()


add_executable(
  ordered_goals_planner

  src/main.cpp
)


target_link_libraries(ordered_goals_planner
  ${ORDEREDGOALSPLANNER_LIBRARIES}
)
