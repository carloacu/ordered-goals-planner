#include "converttoparallelplan.hpp"
#include <list>
#include <set>
#include <orderedgoalsplanner/types/action.hpp>
#include <orderedgoalsplanner/types/actioninvocationwithgoal.hpp>
#include <orderedgoalsplanner/types/actionstodoinparallel.hpp>
#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/types/parallelplan.hpp>
#include <orderedgoalsplanner/types/problem.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/types/worldstate.hpp>
#include <orderedgoalsplanner/orderedgoalsplanner.hpp>
#include "actiondataforparallelisation.hpp"
#include "notifyactiondone.hpp"

namespace ogp
{

namespace
{

void _notifyActionsDoneAndRemoveCorrespondingGoals(std::list<Goal>& pGoals,
                                                   const std::list<ActionDataForParallelisation>& pActions,
                                                   Problem& pProblem,
                                                   const Domain& pDomain,
                                                   const SetOfCallbacks& pCallbacks,
                                                   const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  LookForAnActionOutputInfos lookForAnActionOutputInfos;
  for (auto& currActionTmpData : pActions)
    notifyActionDone(pProblem, pDomain, pCallbacks, currActionTmpData.actionInvWithGoal, pNow, &lookForAnActionOutputInfos);
  std::list<Goal> goalsSatisfied;
  lookForAnActionOutputInfos.moveGoalsDone(goalsSatisfied);
  for (auto& currGoal : goalsSatisfied)
    pGoals.remove(currGoal);
}

}


ParallelPan toParallelPlan
(std::list<ActionInvocationWithGoal>& pSequentialPlan,
 bool pParalleliseOnyFirstStep,
 Problem& pProblem,
 const Domain& pDomain,
 std::list<Goal>& pGoals,
 const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  const auto& actions = pDomain.actions();
  const auto& ontology = pDomain.getOntology();
  std::list<std::list<ActionDataForParallelisation>> currentRes;
  const SetOfCallbacks callbacks;

  // Connvert list of actions to a list of list of actions
  while (!pSequentialPlan.empty())
  {
    ActionInvocationWithGoal& actionInvWithGoal = pSequentialPlan.front();
    auto itAction = actions.find(actionInvWithGoal.actionInvocation.actionId);
    if (itAction == actions.end())
      throw std::runtime_error("ActionId \"" + actionInvWithGoal.actionInvocation.actionId + "\" not found in algorithm to manaage parralelisation");
    std::list<ActionDataForParallelisation> actionInASubList;
    actionInASubList.emplace_back(itAction->second, std::move(actionInvWithGoal));
    currentRes.emplace_back(std::move(actionInASubList));
    pSequentialPlan.pop_front();
  }

  for (auto itPlanStep = currentRes.begin(); itPlanStep != currentRes.end(); ++itPlanStep)
  {
    const Goal* goalPtr = nullptr;
    for (ActionDataForParallelisation& currActionTmpData : *itPlanStep)
    {
      notifyActionStarted(pProblem, pDomain, callbacks, currActionTmpData.actionInvWithGoal, pNow);
      if (goalPtr == nullptr && currActionTmpData.actionInvWithGoal.fromGoal)
        goalPtr = &*currActionTmpData.actionInvWithGoal.fromGoal;
    }
    if (goalPtr == nullptr)
      continue;
    auto& goal = *goalPtr;

    auto itPlanStepCandidate = itPlanStep;
    ++itPlanStepCandidate;
    while (itPlanStepCandidate != currentRes.end())
    {
      if (!itPlanStepCandidate->empty())
      {
        auto& actionInvocationCand = itPlanStepCandidate->front();
        if (!actionInvocationCand.actionInvWithGoal.fromGoal ||
            *actionInvocationCand.actionInvWithGoal.fromGoal != goal)
          break;
        if (actionInvocationCand.canBeInParallelOfList(*itPlanStep))
        {
          const Condition* conditionWithoutParameterPtr = actionInvocationCand.getConditionWithoutParameterPtr();
          if (conditionWithoutParameterPtr == nullptr ||
              conditionWithoutParameterPtr->isTrue(pProblem.worldState, ontology.constants, pProblem.objects))
          {
            auto tmpProblem = pProblem;
            notifyActionStarted(tmpProblem, pDomain, callbacks, actionInvocationCand.actionInvWithGoal, pNow);
            notifyActionDone(tmpProblem, pDomain, callbacks, actionInvocationCand.actionInvWithGoal, pNow);
            auto remainingGoals = pGoals;
            _notifyActionsDoneAndRemoveCorrespondingGoals(remainingGoals, *itPlanStep, tmpProblem, pDomain, callbacks, pNow);

            auto itNextInPlan = itPlanStep;
            ++itNextInPlan;
            auto goalsCand = extractSatisfiedGoals(tmpProblem, pDomain, itNextInPlan, currentRes, &actionInvocationCand, pNow);
            if (goalsCand == remainingGoals)
            {
              notifyActionStarted(pProblem, pDomain, callbacks, actionInvocationCand.actionInvWithGoal, pNow);
              itPlanStep->emplace_back(std::move(actionInvocationCand));
              itPlanStepCandidate = currentRes.erase(itPlanStepCandidate);
              continue;
            }
          }
        }
      }
      ++itPlanStepCandidate;
    }

    if (pParalleliseOnyFirstStep)
      break;
    _notifyActionsDoneAndRemoveCorrespondingGoals(pGoals, *itPlanStep, pProblem, pDomain, callbacks, pNow);
  }

  ParallelPan res;
  for (auto& currResStep : currentRes)
  {
    ActionsToDoInParallel subRes;
    for (auto& currActionTmpData : currResStep)
      subRes.actions.emplace_back(std::move(currActionTmpData.actionInvWithGoal));
    if (!subRes.actions.empty())
      res.actionsToDoInParallel.emplace_back(std::move(subRes));
  }
  return res;
}



} // End of namespace ogp
