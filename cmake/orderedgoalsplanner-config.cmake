get_filename_component(_orderedgoalsplanner_root "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_orderedgoalsplanner_root "${_orderedgoalsplanner_root}/.." ABSOLUTE)

set(ORDEREDGOALSPLANNER_FOUND TRUE)

set(
  ORDEREDGOALSPLANNER_INCLUDE_DIRS
  ${_orderedgoalsplanner_root}/include
  CACHE INTERNAL "" FORCE
)

set(
  ORDEREDGOALSPLANNER_LIBRARIES
  "ordered_goals_planner_lib"
)
