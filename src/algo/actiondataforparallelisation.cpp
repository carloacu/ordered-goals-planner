#include "actiondataforparallelisation.hpp"
#include <orderedgoalsplanner/types/action.hpp>
#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/types/problem.hpp>
#include <orderedgoalsplanner/types/lookforanactionoutputinfos.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>

namespace ogp
{

ActionDataForParallelisation::ActionDataForParallelisation(const Action& pAction, ActionInvocationWithGoal&& pActionInvWithGoal)
  : action(pAction),
    actionInvWithGoal(std::move(pActionInvWithGoal)),
    conditionWithParameterFilled()
{
}

const Condition* ActionDataForParallelisation::getConditionWithoutParameterPtr()
{
  if (!action.precondition)
    return nullptr;
  if (actionInvWithGoal.actionInvocation.parameters.empty())
    return &*action.precondition;
  if (!conditionWithParameterFilled)
    conditionWithParameterFilled = action.precondition->clone(&actionInvWithGoal.actionInvocation.parameters);
  return &*conditionWithParameterFilled;
}


const WorldStateModification* ActionDataForParallelisation::getWorldStateModificationAtStartWithoutParameterPtr()
{
  if (!action.effect.worldStateModificationAtStart)
    return nullptr;
  if (actionInvWithGoal.actionInvocation.parameters.empty())
    return &*action.effect.worldStateModificationAtStart;
  if (!worldStateModificationAtStartWithParameterFilled)
    worldStateModificationAtStartWithParameterFilled = action.effect.worldStateModificationAtStart->clone(&actionInvWithGoal.actionInvocation.parameters);
  return &*worldStateModificationAtStartWithParameterFilled;
}


const WorldStateModification* ActionDataForParallelisation::getWorldStateModificationWithoutParameterPtr()
{
  if (!action.effect.worldStateModification)
    return nullptr;
  if (actionInvWithGoal.actionInvocation.parameters.empty())
    return &*action.effect.worldStateModification;
  if (!worldStateModificationWithParameterFilled)
    worldStateModificationWithParameterFilled = action.effect.worldStateModification->clone(&actionInvWithGoal.actionInvocation.parameters);
  return &*worldStateModificationWithParameterFilled;
}


const WorldStateModification* ActionDataForParallelisation::getPotentialWorldStateModificationWithoutParameterPtr()
{
  if (!action.effect.potentialWorldStateModification)
    return nullptr;
  if (actionInvWithGoal.actionInvocation.parameters.empty())
    return &*action.effect.potentialWorldStateModification;
  if (!potentialWorldStateModificationWithParameterFilled)
    potentialWorldStateModificationWithParameterFilled = action.effect.potentialWorldStateModification->clone(&actionInvWithGoal.actionInvocation.parameters);
  return &*potentialWorldStateModificationWithParameterFilled;
}


const std::set<FactOptional>& ActionDataForParallelisation::getAllOptFactsThatCanBeModified()
{
  if (!factsThatCanBeModifiedPtr)
  {
    factsThatCanBeModifiedPtr = std::make_unique<std::set<FactOptional>>();
    auto addWmToRes = [&](const WorldStateModification* pWmPtr)
    {
      if (pWmPtr)
      {
        pWmPtr->forAllThatCanBeModified([&](const FactOptional& pFactOptional) {
            factsThatCanBeModifiedPtr->insert(pFactOptional);
            return ContinueOrBreak::CONTINUE;
        });
      }
    };
    addWmToRes(getWorldStateModificationAtStartWithoutParameterPtr());
    addWmToRes(getWorldStateModificationWithoutParameterPtr());
    addWmToRes(getPotentialWorldStateModificationWithoutParameterPtr());
  }
  return *factsThatCanBeModifiedPtr;
}


bool ActionDataForParallelisation::hasAContradictionWithAnEffect(const std::set<FactOptional>& pFactsOpt)
{
  auto* effectPtr = getWorldStateModificationAtStartWithoutParameterPtr();
  if (effectPtr != nullptr && effectPtr->hasAContradictionWith(pFactsOpt))
    return true;

  effectPtr = getWorldStateModificationWithoutParameterPtr();
  if (effectPtr != nullptr && effectPtr->hasAContradictionWith(pFactsOpt))
    return true;

  effectPtr = getPotentialWorldStateModificationWithoutParameterPtr();
  if (effectPtr != nullptr && effectPtr->hasAContradictionWith(pFactsOpt))
    return true;
  return false;
}


bool ActionDataForParallelisation::canBeInParallel(ActionDataForParallelisation& pOther)
{
  const auto& effectFacts = getAllOptFactsThatCanBeModified();
  auto* otherConditionPtr = pOther.getConditionWithoutParameterPtr();
  if (otherConditionPtr != nullptr && otherConditionPtr->hasAContradictionWith(effectFacts))
    return false;

  const auto& otherEffectFacts = pOther.getAllOptFactsThatCanBeModified();
  auto* conditionPtr = getConditionWithoutParameterPtr();
  if (conditionPtr != nullptr && conditionPtr->hasAContradictionWith(otherEffectFacts))
    return false;

  if (hasAContradictionWithAnEffect(otherEffectFacts))
    return false;

  return !pOther.hasAContradictionWithAnEffect(effectFacts);
}


bool ActionDataForParallelisation::canBeInParallelOfList(std::list<ActionDataForParallelisation>& pOthers)
{
  for (auto& currOther : pOthers)
    if (!canBeInParallel(currOther))
      return false;
  return true;
}



std::list<Goal> extractSatisfiedGoals(
    Problem& pProblem,
    const Domain& pDomain,
    std::list<std::list<ActionDataForParallelisation>>::iterator pCurrItInPlan,
    std::list<std::list<ActionDataForParallelisation>>& pPlan,
    const ActionDataForParallelisation* pActionToSkipPtr,
    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  std::list<Goal> res;
  auto& setOfEvents = pDomain.getSetOfEvents();
  const SetOfCallbacks emptyCallbacks;
  const auto& ontology = pDomain.getOntology();
  LookForAnActionOutputInfos lookForAnActionOutputInfos;
  while (pCurrItInPlan != pPlan.end())
  {
    std::list<ActionDataForParallelisation*> actionsInParallel;
    for (auto& currAction : *pCurrItInPlan)
    {
      if (&currAction == pActionToSkipPtr)
        continue;

      const auto* conditionPtr = currAction.getConditionWithoutParameterPtr();
      if (conditionPtr != nullptr && !conditionPtr->isTrue(pProblem.worldState, ontology.constants, pProblem.objects))
        return {};

      auto* worldStateModificationAtStartWithoutParameterPtr = currAction.getWorldStateModificationAtStartWithoutParameterPtr();
      if (worldStateModificationAtStartWithoutParameterPtr != nullptr)
        pProblem.worldState.modify(worldStateModificationAtStartWithoutParameterPtr, pProblem.goalStack, setOfEvents,
                                   emptyCallbacks, ontology, pProblem.objects, pNow);
      actionsInParallel.emplace_back(&currAction);

      bool somethingChanged = false;

      const auto* worldStateModificationWithoutParameterPtr = currAction.getWorldStateModificationWithoutParameterPtr();
      if (worldStateModificationWithoutParameterPtr != nullptr)
        somethingChanged = pProblem.worldState.modify(worldStateModificationWithoutParameterPtr, pProblem.goalStack, setOfEvents,
                                                      emptyCallbacks, ontology, pProblem.objects, pNow);

      const auto* potentialWorldStateModificationWithoutParameterPtr = currAction.getPotentialWorldStateModificationWithoutParameterPtr();
      if (potentialWorldStateModificationWithoutParameterPtr != nullptr)
        somethingChanged = pProblem.worldState.modify(potentialWorldStateModificationWithoutParameterPtr, pProblem.goalStack, setOfEvents,
                                                      emptyCallbacks, ontology, pProblem.objects, pNow) || somethingChanged;

      if (!somethingChanged)
        return {};

      pProblem.goalStack.notifyActionDone(currAction.actionInvWithGoal, pNow,
                                          &currAction.action.effect.goalsToAdd,
                                          &currAction.action.effect.goalsToAddInCurrentPriority, pProblem.worldState,
                                          ontology.constants, pProblem.objects, &lookForAnActionOutputInfos);
    }
    ++pCurrItInPlan;
  }

  lookForAnActionOutputInfos.moveGoalsDone(res);
  return res;
}


} // End of namespace ogp
