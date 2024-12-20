#include <orderedgoalsplanner/types/worldstate.hpp>
#include <list>
#include <orderedgoalsplanner/types/goalstack.hpp>
#include <orderedgoalsplanner/types/actioninvocationwithgoal.hpp>
#include <orderedgoalsplanner/types/ontology.hpp>
#include <orderedgoalsplanner/types/setofcallbacks.hpp>
#include <orderedgoalsplanner/types/setoffacts.hpp>
#include <orderedgoalsplanner/types/setofevents.hpp>
#include <orderedgoalsplanner/types/worldstatemodification.hpp>
#include <orderedgoalsplanner/util/util.hpp>
#include <orderedgoalsplanner/util/serializer/deserializefrompddl.hpp>
#include "expressionParsed.hpp"

namespace ogp
{

WorldState::WorldState(const SetOfFacts* pFactsPtr)
  : onFactsChanged(),
    onPunctualFacts(),
    onFactsAdded(),
    onFactsRemoved(),
    _factsMapping(pFactsPtr != nullptr ? *pFactsPtr : SetOfFacts())
{
}


WorldState::WorldState(const WorldState& pOther)
  : onFactsChanged(),
    onPunctualFacts(),
    onFactsAdded(),
    onFactsRemoved(),
    _factsMapping(pOther._factsMapping)
{
}


WorldState::~WorldState()
{
}


void WorldState::operator=(const WorldState& pOther)
{
  _factsMapping = pOther._factsMapping;
}


bool WorldState::modifyFactsFromPddl(const std::string& pStr,
                                     std::size_t& pPos,
                                     GoalStack& pGoalStack,
                                     const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                                     const SetOfCallbacks& pCallbacks,
                                     const Ontology& pOntology,
                                     const SetOfEntities& pEntities,
                                     const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                                     bool pCanFactsBeRemoved)
{
  auto strSize = pStr.size();
  ExpressionParsed::skipSpaces(pStr, pPos);
  WhatChanged whatChanged;

  while (pPos < strSize && pStr[pPos] != ')')
  {
    bool isFactNegated = false;
    Fact fact(pStr, true, pOntology, pEntities, {}, &isFactNegated, pPos, &pPos);
    if (isFactNegated)
      _removeAFact(whatChanged, fact);
    else
      _addAFact(whatChanged, fact, pGoalStack, pSetOfEvents, pCallbacks,
                pOntology, pEntities, pNow, pCanFactsBeRemoved);
  }

  pGoalStack._removeNoStackableGoalsAndNotifyGoalsChanged(*this, pOntology.constants, pEntities, pNow);
  bool goalChanged = false;
  _notifyWhatChanged(whatChanged, goalChanged, pGoalStack, pSetOfEvents, pCallbacks,
                     pOntology, pEntities, pNow);
  return whatChanged.hasFactsToModifyInTheWorldForSure();
}


void WorldState::applyEffect(const std::map<Parameter, Entity>& pParameters,
                             const std::unique_ptr<WorldStateModification>& pEffect,
                             bool& pGoalChanged,
                             GoalStack& pGoalStack,
                             const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                             const SetOfCallbacks& pCallbacks,
                             const Ontology& pOntology,
                             const SetOfEntities& pEntities,
                             const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  const bool canFactsBeRemoved = true;
  WhatChanged whatChanged;
  if (pEffect)
  {
    if (pParameters.empty())
    {
      _modify(whatChanged, &*pEffect, pGoalStack, pSetOfEvents, pCallbacks, pOntology, pEntities, pNow, canFactsBeRemoved);
    }
    else
    {
      auto effect = pEffect->clone(&pParameters);
      _modify(whatChanged, &*effect, pGoalStack, pSetOfEvents, pCallbacks, pOntology, pEntities, pNow, canFactsBeRemoved);
    }
  }

  _notifyWhatChanged(whatChanged, pGoalChanged, pGoalStack, pSetOfEvents,
                     pCallbacks, pOntology, pEntities, pNow);
}


bool WorldState::addFact(const Fact& pFact,
                         GoalStack& pGoalStack,
                         const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                         const SetOfCallbacks& pCallbacks,
                         const Ontology& pOntology,
                         const SetOfEntities& pEntities,
                         const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                         bool pCanFactsBeRemoved)
{
  return addFacts(std::vector<Fact>{pFact}, pGoalStack, pSetOfEvents, pCallbacks,
                  pOntology, pEntities, pNow, pCanFactsBeRemoved);
}

template<typename FACTS>
bool WorldState::addFacts(const FACTS& pFacts,
                          GoalStack& pGoalStack,
                          const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                          const SetOfCallbacks& pCallbacks,
                          const Ontology& pOntology,
                          const SetOfEntities& pEntities,
                          const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                          bool pCanFactsBeRemoved)
{
  WhatChanged whatChanged;
  _addFacts(whatChanged, pFacts, pGoalStack, pSetOfEvents, pCallbacks, pOntology, pEntities, pNow, pCanFactsBeRemoved);
  pGoalStack._removeNoStackableGoalsAndNotifyGoalsChanged(*this, pOntology.constants, pEntities, pNow);
  bool goalChanged = false;
  _notifyWhatChanged(whatChanged, goalChanged, pGoalStack, pSetOfEvents, pCallbacks,
                     pOntology, pEntities, pNow);
  return whatChanged.hasFactsToModifyInTheWorldForSure();
}

template bool WorldState::addFacts<std::set<Fact>>(const std::set<Fact>&, GoalStack&, const std::map<SetOfEventsId, SetOfEvents>&, const SetOfCallbacks&, const Ontology&, const SetOfEntities&, const std::unique_ptr<std::chrono::steady_clock::time_point>&, bool);
template bool WorldState::addFacts<std::vector<Fact>>(const std::vector<Fact>&, GoalStack&, const std::map<SetOfEventsId, SetOfEvents>&, const SetOfCallbacks&, const Ontology&, const SetOfEntities&, const std::unique_ptr<std::chrono::steady_clock::time_point>&, bool);

bool WorldState::hasFact(const Fact& pFact) const
{
  return _factsMapping.facts().count(pFact) > 0;
}

bool WorldState::removeFact(const Fact& pFact,
                            GoalStack& pGoalStack,
                            const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                            const SetOfCallbacks& pCallbacks,
                            const Ontology& pOntology,
                            const SetOfEntities& pEntities,
                            const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  return removeFacts(std::vector<Fact>{pFact}, pGoalStack, pSetOfEvents, pCallbacks, pOntology, pEntities, pNow);
}

template<typename FACTS>
bool WorldState::removeFacts(const FACTS& pFacts,
                             GoalStack& pGoalStack,
                             const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                             const SetOfCallbacks& pCallbacks,
                             const Ontology& pOntology,
                             const SetOfEntities& pEntities,
                             const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  WhatChanged whatChanged;
  _removeFacts(whatChanged, pFacts);
  pGoalStack._removeNoStackableGoalsAndNotifyGoalsChanged(*this, pOntology.constants, pEntities, pNow);
  bool goalChanged = false;
  _notifyWhatChanged(whatChanged, goalChanged, pGoalStack, pSetOfEvents, pCallbacks, pOntology, pEntities, pNow);
  return whatChanged.hasFactsToModifyInTheWorldForSure();
}


template<typename FACTS>
void WorldState::_addFacts(WhatChanged& pWhatChanged,
                           const FACTS& pFacts,
                           GoalStack& pGoalStack,
                           const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                           const SetOfCallbacks& pCallbacks,
                           const Ontology& pOntology,
                           const SetOfEntities& pEntities,
                           const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                           bool pCanFactsBeRemoved)
{
  for (const auto& currFact : pFacts)
    _addAFact(pWhatChanged, currFact, pGoalStack, pSetOfEvents, pCallbacks, pOntology, pEntities, pNow, pCanFactsBeRemoved);
}

template void WorldState::_addFacts<std::set<Fact>>(WhatChanged&, const std::set<Fact>&, GoalStack&, const std::map<SetOfEventsId, SetOfEvents>&, const SetOfCallbacks&, const Ontology&, const SetOfEntities&, const std::unique_ptr<std::chrono::steady_clock::time_point>&, bool);
template void WorldState::_addFacts<std::vector<Fact>>(WhatChanged&, const std::vector<Fact>&, GoalStack&, const std::map<SetOfEventsId, SetOfEvents>&, const SetOfCallbacks&, const Ontology&, const SetOfEntities&, const std::unique_ptr<std::chrono::steady_clock::time_point>&, bool);


void WorldState::_addAFact(WhatChanged& pWhatChanged,
                          const Fact& pFact,
                          GoalStack& pGoalStack,
                          const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                          const SetOfCallbacks& pCallbacks,
                          const Ontology& pOntology,
                          const SetOfEntities& pEntities,
                          const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                          bool pCanFactsBeRemoved)
{
  if (pFact.isPunctual())
  {
    pWhatChanged.punctualFacts.insert(pFact);
    return;
  }
  if (_factsMapping.facts().count(pFact) > 0)
    return;
  bool skipThisFact = false;

  // Remove existing facts if needed
  bool aFactWasRemoved = false;
  do
  {
    aFactWasRemoved = false;
    auto factMatchInWs = _factsMapping.find(pFact, true);
    for (auto itExistingFact = factMatchInWs.begin(); itExistingFact != factMatchInWs.end(); )
    {
      const auto& currExistingFact = *itExistingFact;

      if (pFact.isValueNegated() && !currExistingFact.isValueNegated() && pFact.fluent() != currExistingFact.fluent())
        skipThisFact = true;

      if (pFact.arguments() == currExistingFact.arguments() &&
          ((!pFact.isValueNegated() && !currExistingFact.isValueNegated() && pFact.fluent() != currExistingFact.fluent()) ||
           (pFact.isValueNegated() && !currExistingFact.isValueNegated() && pFact.fluent() == currExistingFact.fluent()) ||
           (!pFact.isValueNegated() && currExistingFact.isValueNegated())))
      {
        WhatChanged subWhatChanged;
        _removeFacts(subWhatChanged, std::vector<ogp::Fact>{currExistingFact});
        pGoalStack._removeNoStackableGoalsAndNotifyGoalsChanged(*this, pOntology.constants, pEntities, pNow);
        bool goalChanged = false;
        _notifyWhatChanged(subWhatChanged, goalChanged, pGoalStack, pSetOfEvents, pCallbacks,
                           pOntology, pEntities, pNow);
        aFactWasRemoved = true;
        break;
      }

      if (skipThisFact)
        break;
      ++itExistingFact;
    }
    if (skipThisFact)
      continue;
  }
  while (aFactWasRemoved);

  if (!skipThisFact)
  {
    pWhatChanged.addedFacts.insert(pFact);
    _factsMapping.add(pFact, pCanFactsBeRemoved);
  }
}

template<typename FACTS>
void WorldState::_removeFacts(WhatChanged& pWhatChanged,
                              const FACTS& pFacts)
{
  for (const auto& currFact : pFacts)
    _removeAFact(pWhatChanged, currFact);
}


void WorldState::_removeAFact(WhatChanged& pWhatChanged,
                              const Fact& pFact)
{
  pWhatChanged.removedFacts.insert(pFact);
  _factsMapping.erase(pFact);
}

void WorldState::_modify(WhatChanged& pWhatChanged,
                         const WorldStateModification* pWsModifPtr,
                         GoalStack& pGoalStack,
                         const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                         const SetOfCallbacks& pCallbacks,
                         const Ontology& pOntology,
                         const SetOfEntities& pEntities,
                         const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                         bool pCanFactsBeRemoved)
{
  if (pWsModifPtr == nullptr)
    return;

  std::list<Fact> factsToAdd;
  std::list<Fact> factsToRemove;
  pWsModifPtr->forAll(
        [&](const FactOptional& pFactOptional)
  {
    if (pFactOptional.isFactNegated)
      factsToRemove.emplace_back(pFactOptional.fact);
    else
      factsToAdd.emplace_back(pFactOptional.fact);
  }, _factsMapping);

  _addFacts(pWhatChanged, factsToAdd, pGoalStack, pSetOfEvents, pCallbacks, pOntology, pEntities, pNow, pCanFactsBeRemoved);
  _removeFacts(pWhatChanged, factsToRemove);
  pGoalStack._removeNoStackableGoalsAndNotifyGoalsChanged(*this, pOntology.constants, pEntities, pNow);
}


bool WorldState::modify(const WorldStateModification* pWsModifPtr,
                        GoalStack& pGoalStack,
                        const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                        const SetOfCallbacks& pCallbacks,
                        const Ontology& pOntology,
                        const SetOfEntities& pEntities,
                        const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow,
                        bool pCanFactsBeRemoved)
{
  WhatChanged whatChanged;
  _modify(whatChanged, pWsModifPtr, pGoalStack, pSetOfEvents, pCallbacks, pOntology, pEntities, pNow, pCanFactsBeRemoved);
  bool goalChanged = false;
  _notifyWhatChanged(whatChanged, goalChanged, pGoalStack, pSetOfEvents, pCallbacks,
                     pOntology, pEntities, pNow);
  return whatChanged.hasFactsToModifyInTheWorldForSure();
}


void WorldState::setFacts(const std::set<Fact>& pFacts,
                          GoalStack& pGoalStack,
                          const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                          const SetOfCallbacks& pCallbacks,
                          const Ontology& pOntology,
                          const SetOfEntities& pEntities,
                          const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  _factsMapping.clear();
  for (const auto& currFact : pFacts)
    _factsMapping.add(currFact);
  WhatChanged whatChanged;
  pGoalStack._removeNoStackableGoalsAndNotifyGoalsChanged(*this, pOntology.constants, pEntities, pNow);
  bool goalChanged = false;
  _notifyWhatChanged(whatChanged, goalChanged, pGoalStack, pSetOfEvents, pCallbacks,
                     pOntology, pEntities, pNow);
}


bool WorldState::isOptionalFactSatisfied(const FactOptional& pFactOptional) const
{
  const auto& facts = _factsMapping.facts();
  return (pFactOptional.isFactNegated || facts.count(pFactOptional.fact) > 0) &&
      (!pFactOptional.isFactNegated || facts.count(pFactOptional.fact) == 0);
}


bool WorldState::isOptionalFactSatisfiedInASpecificContext(const FactOptional& pFactOptional,
                                                           const std::set<Fact>& pPunctualFacts,
                                                           const std::set<Fact>& pRemovedFacts,
                                                           bool pCheckAllPossibilities,
                                                           std::map<Parameter, std::set<Entity>>* pParametersToPossibleArgumentsPtr,
                                                           std::map<Parameter, std::set<Entity>>* pParametersToModifyInPlacePtr,
                                                           bool* pCanBecomeTruePtr) const
{
  if (pFactOptional.fact.isPunctual() && !pFactOptional.isFactNegated)
    return pPunctualFacts.count(pFactOptional.fact) != 0;

  std::map<Parameter, std::set<Entity>> newParameters;
  if (pFactOptional.isFactNegated)
  {
    bool res = pFactOptional.fact.isInOtherFacts(pRemovedFacts, true, &newParameters, pCheckAllPossibilities, pParametersToPossibleArgumentsPtr, pParametersToModifyInPlacePtr);
    if (res)
    {
      if (pParametersToPossibleArgumentsPtr != nullptr)
        applyNewParams(*pParametersToPossibleArgumentsPtr, newParameters);
      return true;
    }

    auto factMatchingInWs = _factsMapping.find(pFactOptional.fact, true);
    if (!factMatchingInWs.empty())
    {
      if (pParametersToPossibleArgumentsPtr != nullptr)
      {
        std::list<std::map<Parameter, Entity>> paramPossibilities;
        unfoldMapWithSet(paramPossibilities, *pParametersToPossibleArgumentsPtr);

        for (auto& currParamPoss : paramPossibilities)
        {
          auto factToCompare = pFactOptional.fact;
          factToCompare.replaceArguments(currParamPoss);
          if (factToCompare.fluent() && factToCompare.fluent()->isAnyValue())
          {
            for (const auto& currFact : factMatchingInWs)
            {
              if (currFact.areEqualExceptAnyValues(factToCompare))
              {
                if (!pFactOptional.fact.fluent() || !pFactOptional.fact.fluent()->isAnyValue())
                {
                  if (pFactOptional.fact.fluent() && currFact.fluent())
                    newParameters = {{pFactOptional.fact.fluent()->toParameter(), {*currFact.fluent()}}};
                  applyNewParams(*pParametersToPossibleArgumentsPtr, newParameters);
                }
                return false;
              }
            }
            return true;
          }
        }
        if (pFactOptional.fact.fluent() && pFactOptional.fact.fluent()->isAnyValue())
          return false;
      }

      if (pFactOptional.fact.fluent() && pFactOptional.fact.fluent()->isAnyValue())
      {
        for (const auto& currFact : factMatchingInWs)
          if (currFact.areEqualExceptAnyValues(pFactOptional.fact))
            return false;
        return true;
      }
    }

    bool triedToMidfyParameters = false;
    if (pFactOptional.fact.isInOtherFactsMap(_factsMapping, true, nullptr, pCheckAllPossibilities, pParametersToPossibleArgumentsPtr, nullptr, &triedToMidfyParameters))
    {
      if (pCanBecomeTruePtr != nullptr && triedToMidfyParameters)
        *pCanBecomeTruePtr = true;
      return false;
    }
    return true;
  }

  auto res = pFactOptional.fact.isInOtherFactsMap(_factsMapping, true, &newParameters, pCheckAllPossibilities, pParametersToPossibleArgumentsPtr,
                                                  pParametersToModifyInPlacePtr);
  if (pParametersToPossibleArgumentsPtr != nullptr)
    applyNewParams(*pParametersToPossibleArgumentsPtr, newParameters);
  return res;
}



bool WorldState::isGoalSatisfied(const Goal& pGoal,
                                 const SetOfEntities& pConstants,
                                 const SetOfEntities& pObjects) const
{
  return pGoal.objective().isTrue(*this, pConstants, pObjects);
}


void WorldState::iterateOnMatchingFactsWithoutFluentConsideration
(const std::function<bool (const Fact&)>& pValueCallback,
 const Fact& pFact,
 const std::map<Parameter, std::set<Entity>>& pParametersToConsiderAsAnyValue,
 const std::map<Parameter, std::set<Entity>>* pParametersToConsiderAsAnyValuePtr) const
{
  auto factMatchInWs = _factsMapping.find(pFact, true);
  for (const auto& currFact : factMatchInWs)
    if (currFact.areEqualExceptAnyValuesAndFluent(pFact, &pParametersToConsiderAsAnyValue, pParametersToConsiderAsAnyValuePtr))
      if (pValueCallback(currFact))
        break;
}


void WorldState::iterateOnMatchingFacts
(const std::function<bool (const Fact&)>& pValueCallback,
 const Fact& pFact,
 const std::map<Parameter, std::set<Entity>>& pParametersToConsiderAsAnyValue,
 const std::map<Parameter, std::set<Entity>>* pParametersToConsiderAsAnyValuePtr) const
{
  auto factMatchInWs = _factsMapping.find(pFact);
  for (const auto& currFact : factMatchInWs)
    if (currFact.areEqualExceptAnyValues(pFact, &pParametersToConsiderAsAnyValue, pParametersToConsiderAsAnyValuePtr))
      if (pValueCallback(currFact))
        break;
}



bool WorldState::_tryToApplyEvent(std::set<EventId>& pEventsAlreadyApplied,
                                  WhatChanged& pWhatChanged,
                                  bool& pGoalChanged,
                                  GoalStack& pGoalStack,
                                  const FactsToValue::ConstMapOfFactIterator& pEventIds,
                                  const std::map<EventId, Event>& pEvents,
                                  const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                                  const SetOfCallbacks& pCallbacks,
                                  const Ontology& pOntology,
                                  const SetOfEntities& pEntities,
                                  const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  const bool canFactsBeRemoved = true;
  bool somethingChanged = false;
  for (const auto& currEventId : pEventIds)
  {
    if (pEventsAlreadyApplied.count(currEventId) == 0)
    {
      pEventsAlreadyApplied.insert(currEventId);
      auto itEvent = pEvents.find(currEventId);
      if (itEvent != pEvents.end())
      {
        const Event& currEvent = itEvent->second;

        std::map<Parameter, std::set<Entity>> parametersToValues;
        for (const auto& currParam : currEvent.parameters)
          parametersToValues[currParam];
        if (!currEvent.precondition || currEvent.precondition->isTrue(*this, pOntology.constants, pEntities,
                                                                      pWhatChanged.punctualFacts, pWhatChanged.removedFacts,
                                                                      &parametersToValues))
        {
          if (currEvent.factsToModify)
          {
            if (!parametersToValues.empty())
            {
              std::list<std::map<Parameter, Entity>> parametersToValuePoss;
              unfoldMapWithSet(parametersToValuePoss, parametersToValues);
              if (!parametersToValuePoss.empty())
              {
                for (const auto& currParamsPoss : parametersToValuePoss)
                {
                  auto factsToModify = currEvent.factsToModify->clone(&currParamsPoss);
                  _modify(pWhatChanged, &*factsToModify, pGoalStack, pSetOfEvents, pCallbacks, pOntology, pEntities, pNow, canFactsBeRemoved);
                }
              }
              else
              {
                const auto* optFactPtr = currEvent.factsToModify->getOptionalFact();
                // If there is no parameter possible value and if the effect is to remove a fact then we remove of the matching facts
                if (optFactPtr != nullptr && optFactPtr->isFactNegated)
                {
                  std::list<const Fact*> factsToRemove;
                  iterateOnMatchingFacts([&](const Fact& pMatchedFact) {
                    factsToRemove.emplace_back(&pMatchedFact);
                    return false;
                  }, optFactPtr->fact, parametersToValues);
                  for (auto& currFactToRemove : factsToRemove)
                    _modify(pWhatChanged, &*strToWsModification("!" + currFactToRemove->toStr(), pOntology, pEntities, {}), // Optimize to construct WorldStateModification without passing by a string
                            pGoalStack, pSetOfEvents, pCallbacks, pOntology, pEntities, pNow, canFactsBeRemoved);
                }
              }
            }
            else
            {
              _modify(pWhatChanged, &*currEvent.factsToModify, pGoalStack, pSetOfEvents, pCallbacks, pOntology, pEntities, pNow, canFactsBeRemoved);
            }
          }
          if (pGoalStack.addGoals(currEvent.goalsToAdd, *this, pOntology.constants, pEntities, pNow))
            pGoalChanged = true;
          somethingChanged = true;
        }
      }
    }
  }
  return somethingChanged;
}



void WorldState::_tryToCallCallbacks(std::set<CallbackId>& pCallbackAlreadyCalled,
                                     const WhatChanged& pWhatChanged,
                                     const SetOfEntities& pConstants,
                                     const SetOfEntities& pObjects,
                                     const FactsToValue::ConstMapOfFactIterator& pCallbackIds,
                                     const std::map<CallbackId, ConditionToCallback>& pCallbacks)
{
  for (const auto& currCallbackId : pCallbackIds)
  {
    if (pCallbackAlreadyCalled.count(currCallbackId) == 0)
    {
      auto itCallback = pCallbacks.find(currCallbackId);
      if (itCallback != pCallbacks.end())
      {
        const ConditionToCallback& currCallback = itCallback->second;

        std::map<Parameter, std::set<Entity>> parametersToValues;
        for (const auto& currParam : currCallback.parameters)
          parametersToValues[currParam];
        if (currCallback.condition && currCallback.condition->isTrue(*this, pConstants, pObjects,
                                                                     pWhatChanged.punctualFacts, pWhatChanged.removedFacts,
                                                                     &parametersToValues))
        {
          pCallbackAlreadyCalled.insert(currCallbackId);
          currCallback.callback();
        }
      }
    }
  }
}

void WorldState::_notifyWhatChanged(WhatChanged& pWhatChanged,
                                    bool& pGoalChanged,
                                    GoalStack& pGoalStack,
                                    const std::map<SetOfEventsId, SetOfEvents>& pSetOfEvents,
                                    const SetOfCallbacks& pCallbacks,
                                    const Ontology& pOntology,
                                    const SetOfEntities& pEntities,
                                    const std::unique_ptr<std::chrono::steady_clock::time_point>& pNow)
{
  if (pWhatChanged.somethingChanged())
  {
    // manage the events
    std::map<SetOfEventsId, std::set<EventId>> soeToEventsAlreadyApplied;
    std::set<CallbackId> callbackAlreadyCalled;
    bool needAnotherLoop = true;
    while (needAnotherLoop)
    {
      needAnotherLoop = false;
      for (auto& currSetOfEvents : pSetOfEvents)
      {
        auto& events = currSetOfEvents.second.events();
        auto& condToReachableEvents = currSetOfEvents.second.reachableEventLinks().conditionToEvents;
        auto& notCondToReachableEvents = currSetOfEvents.second.reachableEventLinks().notConditionToEvents;
        auto& eventsAlreadyApplied = soeToEventsAlreadyApplied[currSetOfEvents.first];

        for (auto& currAddedFact : pWhatChanged.punctualFacts)
        {
          auto it = condToReachableEvents.find(currAddedFact);
          if (_tryToApplyEvent(eventsAlreadyApplied, pWhatChanged, pGoalChanged, pGoalStack, it, events,
                               pSetOfEvents, pCallbacks, pOntology, pEntities, pNow))
            needAnotherLoop = true;
        }
        for (auto& currAddedFact : pWhatChanged.addedFacts)
        {
          auto it = condToReachableEvents.find(currAddedFact);
          if (_tryToApplyEvent(eventsAlreadyApplied, pWhatChanged, pGoalChanged, pGoalStack, it, events,
                               pSetOfEvents, pCallbacks, pOntology, pEntities, pNow))
            needAnotherLoop = true;
        }
        for (auto& currRemovedFact : pWhatChanged.removedFacts)
        {
          auto it = notCondToReachableEvents.find(currRemovedFact);
          if (_tryToApplyEvent(eventsAlreadyApplied, pWhatChanged, pGoalChanged, pGoalStack, it, events,
                               pSetOfEvents, pCallbacks, pOntology, pEntities, pNow))
            needAnotherLoop = true;
        }
      }

      if (!pCallbacks.empty())
      {
        auto& callbacks = pCallbacks.callbacks();
        auto& condToReachableCallbacks = pCallbacks.reachableCallbackLinks().conditionToCallbacks;
        auto& notCondToReachableCallbacks = pCallbacks.reachableCallbackLinks().notConditionToCallbacks;
        for (auto& currAddedFact : pWhatChanged.punctualFacts)
        {
          auto it = condToReachableCallbacks.find(currAddedFact);
          _tryToCallCallbacks(callbackAlreadyCalled, pWhatChanged, pOntology.constants, pEntities, it, callbacks);
        }
        for (auto& currAddedFact : pWhatChanged.addedFacts)
        {
          auto it = condToReachableCallbacks.find(currAddedFact);
          _tryToCallCallbacks(callbackAlreadyCalled, pWhatChanged, pOntology.constants, pEntities, it, callbacks);
        }
        for (auto& currRemovedFact : pWhatChanged.removedFacts)
        {
          auto it = notCondToReachableCallbacks.find(currRemovedFact);
          _tryToCallCallbacks(callbackAlreadyCalled, pWhatChanged, pOntology.constants, pEntities, it, callbacks);
        }
      }
    }

    if (!pWhatChanged.punctualFacts.empty())
      onPunctualFacts(pWhatChanged.punctualFacts);
    if (!pWhatChanged.addedFacts.empty())
      onFactsAdded(pWhatChanged.addedFacts);
    if (!pWhatChanged.removedFacts.empty())
      onFactsRemoved(pWhatChanged.removedFacts);
    if (pWhatChanged.hasFactsToModifyInTheWorldForSure())
      onFactsChanged(_factsMapping.facts());
  }
}


} // !ogp
