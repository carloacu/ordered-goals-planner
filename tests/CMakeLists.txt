cmake_minimum_required(VERSION 3.8)

project(ordered_goals_planner_tests)

include_directories(".")

if(NOT ORDEREDGOALSPLANNER_FOUND)
  find_package(ordered_goals_planner_lib)
endif()

enable_testing()
find_package(GTest REQUIRED)


add_executable(
  ordered_goals_planner_tests

  src/plannerusingexternaldata.hpp

  src/test_arithmeticevaluator.cpp
  src/test_callbacks.cpp
  src/test_conditional_effects_requirement.cpp
  src/test_evaluate.cpp
  src/test_existential_preconditions_requirement.cpp
  src/test_facttoconditions.cpp
  src/test_goalscache.cpp
  src/test_planner.cpp
  src/test_plannerWithSingleType.cpp
  src/test_ontology.cpp
  src/test_parallelplan.cpp
  src/test_pddl_serialization.cpp
  src/test_setoffacts.cpp
  src/test_successionscache.cpp
  src/test_universal_preconditions_requirement.cpp
  src/test_worldstate.cpp
  src/docexamples/test_planningDummyExample.hpp
  src/docexamples/test_planningDummyExample.cpp
  src/docexamples/test_planningExampleWithAPreconditionSolve.hpp
  src/docexamples/test_planningExampleWithAPreconditionSolve.cpp
  src/test_util.cpp
  src/test_pddl_serialization.cpp
  src/test_problems_in_data.cpp
  src/main.cpp
)


target_link_libraries(ordered_goals_planner_tests
  ${GTEST_LIBRARIES}
  ${ORDEREDGOALSPLANNER_LIBRARIES}
)

add_test(
  NAME ordered_goals_planner_tests
  COMMAND ordered_goals_planner_tests
)
