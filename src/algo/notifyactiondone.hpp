#ifndef ORDEREDGOALSPLANNER_SRC_ALGO_NOTIFYACTIONDONE_HPP
#define ORDEREDGOALSPLANNER_SRC_ALGO_NOTIFYACTIONDONE_HPP


#include <chrono>
#include <map>
#include <memory>
#include <vector>
#include <orderedgoalsplanner/util/alias.hpp>

namespace ogp
{
struct Action;
struct ActionInvocationWithGoal;
struct Domain;
struct Goal;
struct Problem;
struct Historical;
struct LookForAnActionOutputInfos;
struct SetOfEvents;
struct WorldStateModification;
struct Ontology;
struct SetOfCallbacks;
struct SetOfEntities;


void notifyActionInvocationDone(Problem& pProblem,
                                bool& pGoalChanged,
                                const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                                const SetOfCallbacks& pCallbacks,
                                const ActionInvocationWithGoal& pOnStepOfPlannerResult,
                                const std::unique_ptr<WorldStateModification>& pEffect,
                                const Domain& pDomain,
                                const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                const std::map<int, std::vector<Goal>>* pGoalsToAdd,
                                const std::vector<Goal>* pGoalsToAddInCurrentPriority,
                                LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr);


void updateProblemForNextPotentialPlannerResult(
    Problem& pProblem,
    bool& pGoalChanged,
    const ActionInvocationWithGoal& pOneStepOfPlannerResult,
    const Domain& pDomain,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    Historical* pGlobalHistorical,
    LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr);


void updateProblemForNextPotentialPlannerResultWithAction(
    Problem& pProblem,
    bool& pGoalChanged,
    const ActionInvocationWithGoal& pOneStepOfPlannerResult,
    const Action& pOneStepAction,
    const Domain& pDomain,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    Historical* pGlobalHistorical,
    LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr);


} // End of namespace ogp


#endif // ORDEREDGOALSPLANNER_SRC_ALGO_NOTIFYACTIONDONE_HPP
