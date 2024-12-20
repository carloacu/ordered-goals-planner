project(orderedgoalsplanner)
cmake_minimum_required(VERSION 3.8)

option(
  CMAKE_INSTALL_INCLUDEDIR
  "Where to install the includes"
  include
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(orderedgoalsplanner-config.cmake)

# Making a lib
set(ORDERED_GOALS_PLANNER_HPPS
    include/orderedgoalsplanner/types/action.hpp
    include/orderedgoalsplanner/types/actioninvocation.hpp
    include/orderedgoalsplanner/types/actioninvocationwithgoal.hpp
    include/orderedgoalsplanner/types/actionstodoinparallel.hpp
    include/orderedgoalsplanner/types/axiom.hpp
    include/orderedgoalsplanner/types/condition.hpp
    include/orderedgoalsplanner/types/conditiontocallback.hpp
    include/orderedgoalsplanner/types/condtionstovalue.hpp
    include/orderedgoalsplanner/types/derivedpredicate.hpp
    include/orderedgoalsplanner/types/domain.hpp
    include/orderedgoalsplanner/types/entity.hpp
    include/orderedgoalsplanner/types/event.hpp
    include/orderedgoalsplanner/types/fact.hpp
    include/orderedgoalsplanner/types/factoptional.hpp
    include/orderedgoalsplanner/types/goal.hpp
    include/orderedgoalsplanner/types/goalstack.hpp
    include/orderedgoalsplanner/types/historical.hpp
    include/orderedgoalsplanner/types/lookforanactionoutputinfos.hpp
    include/orderedgoalsplanner/types/factstovalue.hpp
    include/orderedgoalsplanner/types/ontology.hpp
    include/orderedgoalsplanner/types/parameter.hpp
    include/orderedgoalsplanner/types/parallelplan.hpp
    include/orderedgoalsplanner/types/predicate.hpp
    include/orderedgoalsplanner/types/problem.hpp
    include/orderedgoalsplanner/types/problemmodification.hpp
    include/orderedgoalsplanner/types/setofconstfacts.hpp
    include/orderedgoalsplanner/types/setofderivedpredicates.hpp
    include/orderedgoalsplanner/types/setofentities.hpp
    include/orderedgoalsplanner/types/setofevents.hpp
    include/orderedgoalsplanner/types/setoffacts.hpp
    include/orderedgoalsplanner/types/setofpredicates.hpp
    include/orderedgoalsplanner/types/setofderivedpredicates.hpp
    include/orderedgoalsplanner/types/setoftypes.hpp
    include/orderedgoalsplanner/types/setofcallbacks.hpp
    include/orderedgoalsplanner/types/setofconstfacts.hpp
    include/orderedgoalsplanner/types/type.hpp
    include/orderedgoalsplanner/types/worldstate.hpp
    include/orderedgoalsplanner/types/worldstatemodification.hpp
    include/orderedgoalsplanner/util/exportsymbols/macro.hpp
    include/orderedgoalsplanner/util/serializer/deserializefrompddl.hpp
    include/orderedgoalsplanner/util/serializer/serializeinpddl.hpp
    include/orderedgoalsplanner/util/trackers/goalsremovedtracker.hpp
    include/orderedgoalsplanner/util/api.hpp
    include/orderedgoalsplanner/util/alias.hpp
    include/orderedgoalsplanner/util/arithmeticevaluator.hpp
    include/orderedgoalsplanner/util/continueorbreak.hpp
    include/orderedgoalsplanner/util/print.hpp
    include/orderedgoalsplanner/util/observableunsafe.hpp
    include/orderedgoalsplanner/util/replacevariables.hpp
    include/orderedgoalsplanner/util/util.hpp
    include/orderedgoalsplanner/orderedgoalsplanner.hpp
)


set(ORDERED_GOALS_PLANNER_SRCS
    src/algo/actiondataforparallelisation.hpp
    src/algo/actiondataforparallelisation.cpp
    src/algo/converttoparallelplan.hpp
    src/algo/converttoparallelplan.cpp
    src/algo/notifyactiondone.hpp
    src/algo/notifyactiondone.cpp
    src/types/action.cpp
    src/types/actioninvocation.cpp
    src/types/actioninvocationwithgoal.cpp
    src/types/axiom.cpp
    src/types/condition.cpp
    src/types/condtionstovalue.cpp
    src/types/derivedpredicate.cpp
    src/types/domain.cpp
    src/types/entity.cpp
    src/types/expressionParsed.hpp
    src/types/expressionParsed.cpp
    src/types/event.cpp
    src/types/fact.cpp
    src/types/factoptional.cpp
    src/types/factsalreadychecked.hpp
    src/types/goal.cpp
    src/types/goalstack.cpp
    src/types/historical.cpp
    src/types/lookforanactionoutputinfos.cpp
    src/types/factstovalue.cpp
    src/types/parallelplan.cpp
    src/types/parameter.cpp
    src/types/predicate.cpp
    src/types/problemmodification.cpp
    src/types/setofcallbacks.cpp
    src/types/setofentities.cpp
    src/types/setofevents.cpp
    src/types/setoffacts.cpp
    src/types/setofderivedpredicates.cpp
    src/types/setofpredicates.cpp
    src/types/setoftypes.cpp
    src/types/treeofalreadydonepaths.hpp
    src/types/treeofalreadydonepaths.cpp
    src/types/type.cpp
    src/types/worldstate.cpp
    src/types/worldstatemodification.cpp
    src/types/worldstatemodificationprivate.hpp
    src/types/worldstatemodificationprivate.cpp
    src/util/serializer/deserializefrompddl.cpp
    src/util/serializer/serializeinpddl.cpp
    src/util/trackers/goalsremovedtracker.cpp
    src/util/arithmeticevaluator.cpp
    src/util/print.cpp
    src/util/replacevariables.cpp
    src/util/util.cpp
    src/util/uuid.hpp
    src/util/uuid.cpp
    src/orderedgoalsplanner.cpp
)


include_directories(include)


add_library(ordered_goals_planner_lib STATIC
  ${ORDERED_GOALS_PLANNER_HPPS}
  ${ORDERED_GOALS_PLANNER_SRCS}
)
target_compile_features(ordered_goals_planner_lib PRIVATE cxx_std_14)

target_include_directories(ordered_goals_planner_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Create a proper installed package that can be included.
install(TARGETS ordered_goals_planner_lib
    EXPORT ordered_goals_planner_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/orderedgoalsplanner DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT ordered_goals_planner_lib
    FILE ordered_goals_planner_libTargets.cmake
    NAMESPACE ogp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ordered_goals_planner
)

add_subdirectory(bin)


if (BUILD_ORDERED_GOALS_PLANNER_TESTS)
   add_subdirectory(tests)
endif()
