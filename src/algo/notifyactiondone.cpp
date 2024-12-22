#include "notifyactiondone.hpp"
#include <orderedgoalsplanner/types/action.hpp>
#include <orderedgoalsplanner/types/actioninvocationwithgoal.hpp>
#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/types/problem.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/types/worldstate.hpp>

namespace ogp
{

void notifyActionInvocationDone(Problem& pProblem,
                                bool& pGoalChanged,
                                const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                                const SetOfCallbacks& pCallbacks,
                                const ActionInvocationWithGoal& pOnStepOfPlannerResult,
                                const std::unique_ptr<WorldStateModification>& pEffect,
                                const Ontology& pOntology,
                                const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                const std::map<int, std::vector<Goal>>* pGoalsToAdd,
                                const std::vector<Goal>* pGoalsToAddInCurrentPriority,
                                LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr)
{
  pProblem.historical.notifyActionDone(pOnStepOfPlannerResult.actionInvocation.actionId);

  pProblem.worldState.applyEffect(pOnStepOfPlannerResult.actionInvocation.parameters, pEffect,
                                  pGoalChanged, pProblem.goalStack, pSetOfEvents, pCallbacks,
                                  pOntology, pProblem.objects, pNow);

  pGoalChanged = pProblem.goalStack.notifyActionDone(pOnStepOfPlannerResult, pNow, pGoalsToAdd,
                                                     pGoalsToAddInCurrentPriority, pProblem.worldState,
                                                     pOntology.constants, pProblem.objects,
                                                     pLookForAnActionOutputInfosPtr) || pGoalChanged;
}



void updateProblemForNextPotentialPlannerResult(
    Problem& pProblem,
    bool& pGoalChanged,
    const ActionInvocationWithGoal& pOneStepOfPlannerResult,
    const Domain& pDomain,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    Historical* pGlobalHistorical,
    LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr)
{
  auto* actionPtr = pDomain.getActionPtr(pOneStepOfPlannerResult.actionInvocation.actionId);
  if (actionPtr != nullptr)
    updateProblemForNextPotentialPlannerResultWithAction(pProblem, pGoalChanged,
                                                         pOneStepOfPlannerResult, *actionPtr,
                                                         pDomain, pNow, pGlobalHistorical,
                                                         pLookForAnActionOutputInfosPtr);
}


void updateProblemForNextPotentialPlannerResultWithAction(
    Problem& pProblem,
    bool& pGoalChanged,
    const ActionInvocationWithGoal& pOneStepOfPlannerResult,
    const Action& pOneStepAction,
    const Domain& pDomain,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
    Historical* pGlobalHistorical,
    LookForAnActionOutputInfos* pLookForAnActionOutputInfosPtr)
{
  if (pGlobalHistorical != nullptr)
    pGlobalHistorical->notifyActionDone(pOneStepOfPlannerResult.actionInvocation.actionId);
  auto& setOfEvents = pDomain.getSetOfEvents();
  const SetOfCallbacks callbacks;

  const auto& ontology = pDomain.getOntology();
  if (pOneStepAction.effect.worldStateModificationAtStart)
    pProblem.worldState.modify(&*pOneStepAction.effect.worldStateModificationAtStart, pProblem.goalStack, setOfEvents,
                               callbacks, ontology, pProblem.objects, pNow);

  notifyActionInvocationDone(pProblem, pGoalChanged, setOfEvents, callbacks, pOneStepOfPlannerResult,
                             pOneStepAction.effect.worldStateModification, ontology, pNow,
                             &pOneStepAction.effect.goalsToAdd, &pOneStepAction.effect.goalsToAddInCurrentPriority,
                             pLookForAnActionOutputInfosPtr);

  if (pOneStepAction.effect.potentialWorldStateModification)
  {
    auto potentialEffect = pOneStepAction.effect.potentialWorldStateModification->clone(&pOneStepOfPlannerResult.actionInvocation.parameters);
    if (potentialEffect)
      pProblem.worldState.modify(&*potentialEffect, pProblem.goalStack, setOfEvents,
                                 callbacks, ontology, pProblem.objects, pNow);
  }
}


} // End of namespace ogp
