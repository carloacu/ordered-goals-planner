#ifndef ORDEREDGOALSPLANNER_SRC_ALGO_CONVERTTOPARALLELPLAN_HPP
#define ORDEREDGOALSPLANNER_SRC_ALGO_CONVERTTOPARALLELPLAN_HPP

#include <chrono>
#include <list>
#include <map>
#include <memory>
#include <orderedgoalsplanner/util/alias.hpp>

namespace ogp
{
struct ActionInvocationWithGoal;
struct Domain;
struct Goal;
struct ParallelPlan;
struct Problem;
struct WorldState;


ParallelPlan toParallelPlan
(std::list<ActionInvocationWithGoal>& pSequentialPlan,
 bool pParalleliseOnyFirstStep,
 Problem& pProblem,
 const Domain& pDomain,
 std::list<Goal>& pGoals,
 const std::unique_ptr<std::chrono::_V2::steady_clock::time_point>& pNow);

} // End of namespace ogp


#endif // ORDEREDGOALSPLANNER_SRC_ALGO_CONVERTTOPARALLELPLAN_HPP
