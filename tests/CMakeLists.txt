enable_testing()
find_package(GTest REQUIRED)

add_executable(ordered_goals_planner_tests
  src/plannerusingexternaldata.hpp

  src/test_arithmeticevaluator.cpp
  src/test_callbacks.cpp
  src/test_condition.cpp
  src/test_conditional_effects_requirement.cpp
  src/test_evaluate.cpp
  src/test_existential_preconditions_requirement.cpp
  src/test_duration.cpp
  src/test_extractminmaxvalueforfacts.cpp
  src/test_fact.cpp
  src/test_factoptionalstoid.cpp
  src/test_fluent_requirement.cpp
  src/test_goalscache.cpp
  src/test_negative_preconditions_requirement.cpp
  src/test_numeric_objectives.cpp
  src/test_planner.cpp
  src/test_plannerWithSingleType.cpp
  src/test_ontology.cpp
  src/test_ordered_goals_requirement.cpp
  src/test_parallelplan.cpp
  src/test_pddl_serialization.cpp
  src/test_examples.cpp
  src/test_setofentities.cpp
  src/test_setoffacts.cpp
  src/test_successionscache.cpp
  src/test_universal_preconditions_requirement.cpp
  src/test_worldstate.cpp
  src/docexamples/test_planningDummyExample.hpp
  src/docexamples/test_planningDummyExample.cpp
  src/docexamples/test_planningExampleWithAPreconditionSolve.hpp
  src/docexamples/test_planningExampleWithAPreconditionSolve.cpp
  src/test_util.cpp
  src/main.cpp
)

target_link_libraries(ordered_goals_planner_tests
  PRIVATE
    GTest::GTest
    ordered_goals_planner_lib
)

add_test(
  NAME ordered_goals_planner_tests
  COMMAND ordered_goals_planner_tests
)
