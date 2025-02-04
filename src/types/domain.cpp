#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/types/condition.hpp>
#include <orderedgoalsplanner/types/worldstate.hpp>
#include <orderedgoalsplanner/util/util.hpp>
#include "../util/uuid.hpp"
#include "expressionParsed.hpp"

namespace ogp
{
namespace
{
static const ParameterValuesWithConstraints _emptyParametersWithValues;
static const std::vector<Parameter> _emptyParameters;


std::set<std::string> _requirementsManaged = {
  ":strips", ":typing", ":negative-preconditions", ":equality",
  ":existential-preconditions", ":universal-preconditions", ":quantified-preconditions",
  ":conditional-effects", ":fluents", ":numeric-fluents", ":object-fluents",
  ":adl", ":durative-actions",
  ":derived-predicates", ":domain-axioms",
  ":ordered-goals"
};


struct ActionWithConditionAndFactFacts
{
  ActionWithConditionAndFactFacts(const ActionId& pActionId, Action& pAction)
    : actionId(pActionId),
      action(pAction),
      factsFromCondition(),
      factsFromEffect(),
      invertSuccessionsFromActions(),
      invertSuccessionsFromEvents()
  {
  }

  bool isImpossibleSuccession(const ActionWithConditionAndFactFacts& pOther) const
  {
    for (auto& effectOptFact : factsFromEffect)
      if (!effectOptFact.fact.hasAParameter(false))
        for (auto& otherCondOptFact : pOther.factsFromCondition)
          if (effectOptFact.isFactNegated != otherCondOptFact.isFactNegated &&
              effectOptFact.fact == otherCondOptFact.fact)
            return true;
    return false;
  }

  bool doesSuccessionsHasAnInterest(const ActionWithConditionAndFactFacts& pOther) const
  {
    for (auto& effectOptFact : factsFromEffect)
    {
      if (effectOptFact.fact.hasAParameter(true))
        return true;

      if (effectOptFact.fact.value() && effectOptFact.fact.value()->isAnyEntity())
        for (auto& otherCondOptFact : pOther.factsFromCondition)
          if (effectOptFact.isFactNegated == otherCondOptFact.isFactNegated &&
              effectOptFact.fact.areEqualExceptAnyEntitiesAndValue(otherCondOptFact.fact))
            return true;

      if (!effectOptFact.fact.value() || !effectOptFact.fact.value()->isAParameterToFill())
        for (auto& otherCondOptFact : pOther.factsFromCondition)
          if (effectOptFact.isFactNegated != otherCondOptFact.isFactNegated &&
              effectOptFact.fact == otherCondOptFact.fact)
            return false;

      bool hasAnInterest = false;
      for (auto& otherEffectOptFact : pOther.factsFromEffect)
      {
        if (!effectOptFact.doesFactEffectOfSuccessorGiveAnInterestForSuccessor(otherEffectOptFact))
        {
          hasAnInterest = false;
          break;
        }
        else
        {
          hasAnInterest = true;
        }
      }
      if (hasAnInterest)
        return true;
    }
    return false;
  }

  ActionId actionId;
  Action& action;
  std::set<FactOptional> factsFromCondition;
  std::set<FactOptional> factsFromEffect;
  std::set<ActionId> invertSuccessionsFromActions;
  std::set<FullEventId> invertSuccessionsFromEvents;
};


struct EventWithTmpData
{
  EventWithTmpData(const SetOfEventsId& pSetOfEventsId, const EventId& pEventId, Event& pEvent)
    : setOfEventsId(pSetOfEventsId),
      eventId(pEventId),
      event(pEvent),
      invertSuccessionsFromActions(),
      invertSuccessionsFromEvents()
  {
  }

  SetOfEventsId setOfEventsId;
  EventId eventId;
  Event& event;
  std::set<ActionId> invertSuccessionsFromActions;
  std::set<FullEventId> invertSuccessionsFromEvents;
};


void _updateActionsPredecessors(
    std::set<ActionId>& pActions,
    std::set<FullEventId>& pEvents,
    const std::set<ActionId>& pInvertSuccessionsFromActions,
    const std::set<FullEventId>& pInvertSuccessionsFromEvents,
    const std::map<ActionId, ActionWithConditionAndFactFacts>& pAllActionsTmpData,
    const std::map<FullEventId, EventWithTmpData>& pAllEventsTmpData)
{
  for (auto& currActionId : pInvertSuccessionsFromActions)
  {
    if (pActions.count(currActionId) > 0)
      continue;
    pActions.insert(currActionId);

    auto it = pAllActionsTmpData.find(currActionId);
    if (it == pAllActionsTmpData.end())
      throw std::runtime_error("Action predecessor not foud: " + currActionId);
    _updateActionsPredecessors(pActions, pEvents,
                               it->second.invertSuccessionsFromActions,
                               it->second.invertSuccessionsFromEvents,
                               pAllActionsTmpData, pAllEventsTmpData);
  }

  for (auto& currFullEventId : pInvertSuccessionsFromEvents)
  {
    if (pEvents.count(currFullEventId) > 0)
      continue;
    pEvents.insert(currFullEventId);

    auto it = pAllEventsTmpData.find(currFullEventId);
    if (it == pAllEventsTmpData.end())
      throw std::runtime_error("Event predecessor not foud: " + currFullEventId);
    _updateActionsPredecessors(pActions, pEvents,
                               it->second.invertSuccessionsFromActions,
                               it->second.invertSuccessionsFromEvents,
                               pAllActionsTmpData, pAllEventsTmpData);
  }
}

}

Domain::Domain()
  : _uuid(),
    _name(),
    _ontology(),
    _timelessFacts(),
    _actions(),
    _conditionsToActions(),
    _setOfEvents(),
    _requirements()
{
}


Domain::Domain(const std::map<ActionId, Action>& pActions,
               const Ontology& pOntology,
               const SetOfEvents& pSetOfEvents,
               const std::map<SetOfEventsId, SetOfEvents>& pIdToSetOfEvents,
               const SetOfConstFacts& pTimelessFacts,
               const std::string& pName)
  : _uuid(generateUuid()),
    _name(pName),
    _ontology(pOntology),
    _timelessFacts(pTimelessFacts),
    _actions(),
    _conditionsToActions(),
    _setOfEvents(pIdToSetOfEvents),
    _requirements()
{
  for (const auto& currAction : pActions)
    _addAction(currAction.first, currAction.second);

  if (!pSetOfEvents.empty())
    _setOfEvents.emplace(getSetOfEventsIdFromConstructor(), pSetOfEvents);

  _updateSuccessions();
}


void Domain::addAction(const ActionId& pActionId,
                       const Action& pAction)
{
  _addAction(pActionId, pAction);
  _updateSuccessions();
}


void Domain::_addAction(const ActionId& pActionId,
                        const Action& pAction)
{
  if (_actions.count(pActionId) > 0 ||
      pAction.effect.empty())
    return;
  Action clonedAction = pAction.clone(_ontology.derivedPredicates);

  if (clonedAction.canThisActionBeUsedByThePlanner)
  {
    const auto& constFacts = _timelessFacts.setOfFacts();
    if (!clonedAction.effect.worldStateModification &&
             !clonedAction.effect.potentialWorldStateModification)
      clonedAction.canThisActionBeUsedByThePlanner = false;
    else if (!constFacts.empty() &&
             clonedAction.precondition &&
             !clonedAction.precondition->untilFalse([&](const FactOptional& pFactOptional) {
               return !(pFactOptional.isFactNegated && !constFacts.find(pFactOptional.fact).empty());
             }, constFacts))
      clonedAction.canThisActionBeUsedByThePlanner = false;
  }

  const Action& action = _actions.emplace(pActionId, std::move(clonedAction)).first->second;
  if (!action.canThisActionBeUsedByThePlanner)
    return;

  _uuid = generateUuid(); // Regenerate uuid to force the problem to refresh his cache when it will use this object
}

void Domain::removeAction(const ActionId& pActionId)
{
  auto it = _actions.find(pActionId);
  if (it == _actions.end())
    return;
  _uuid = generateUuid(); // Regenerate uuid to force the problem to refresh his cache when it will use this object
  _actions.erase(it);
  _updateSuccessions();
}

const Action* Domain::getActionPtr(const ActionId& pActionId) const
{
  auto it = _actions.find(pActionId);
  if (it != _actions.end())
    return &it->second;
  return nullptr;
}


SetOfEventsId Domain::addSetOfEvents(const SetOfEvents& pSetOfEvents,
                                     const SetOfEventsId& pSetOfEventsId)
{
  _uuid = generateUuid(); // Regenerate uuid to force the problem to refresh his cache when it will use this object
  auto isIdOkForInsertion = [this](const std::string& pId)
  {
    return _setOfEvents.count(pId) == 0;
  };

  auto newId = incrementLastNumberUntilAConditionIsSatisfied(pSetOfEventsId, isIdOkForInsertion);
  _setOfEvents.emplace(newId, pSetOfEvents);
  _updateSuccessions();
  return newId;
}


void Domain::removeSetOfEvents(const SetOfEventsId& pSetOfEventsId)
{
  auto it = _setOfEvents.find(pSetOfEventsId);
  if (it != _setOfEvents.end())
  {
    _uuid = generateUuid(); // Regenerate uuid to force the problem to refresh his cache when it will use this object
    _setOfEvents.erase(it);
    _updateSuccessions();
  }
}

void Domain::clearEvents()
{
  if (!_setOfEvents.empty())
  {
    _uuid = generateUuid(); // Regenerate uuid to force the problem to refresh his cache when it will use this object
    _setOfEvents.clear();
    _updateSuccessions();
  }
}


std::string Domain::printSuccessionCache() const
{
  std::string res;
  for (const auto& currAction : _actions)
  {
    const Action& action = currAction.second;
    auto sc = action.printSuccessionCache();
    if (!sc.empty())
    {
      if (!res.empty())
        res += "\n\n";
      res += "action: " + currAction.first + "\n";
      res += "----------------------------------\n\n";
      res += sc;
    }
  }

  for (const auto& currSetOfEv : _setOfEvents)
  {
    for (const auto& currEv : currSetOfEv.second.events())
    {
      const Event& event = currEv.second;
      auto sc = event.printSuccessionCache();
      if (!sc.empty())
      {
        if (!res.empty())
          res += "\n\n";
        res += "event: " + currSetOfEv.first + "|" + currEv.first + "\n";
        res += "----------------------------------\n\n";
        res += sc;
      }
    }
  }

  return res;
}


void Domain::addRequirement(const std::string& pRequirement)
{
  if (_requirementsManaged.count(pRequirement) == 0)
    throw std::runtime_error("Requirement \"" + pRequirement + "\" is not managed!");
  _requirements.insert(pRequirement);
}


void Domain::_updateSuccessions()
{
  // Update condition to action links
  _conditionsToActions = FactOptionalsToId();
  for (auto& currAction : _actions)
  {
    Action& action = currAction.second;
    if (!action.canThisActionBeUsedByThePlanner)
      continue;

    if (action.precondition)
      _conditionsToActions.add(*action.precondition, currAction.first);
  }

  std::map<ActionId, ActionWithConditionAndFactFacts> actionTmpData;
  std::map<FullEventId, EventWithTmpData> eventTmpData;
  // Add successions cache of the actions
  for (auto& currAction : _actions)
  {
    Action& action = currAction.second;
    if (!action.canThisActionBeUsedByThePlanner)
      continue;

    ActionWithConditionAndFactFacts tmpData(currAction.first, action);
    tmpData.factsFromCondition = action.precondition ? action.precondition->getAllOptFacts() : std::set<FactOptional>();
    tmpData.factsFromEffect = action.effect.getAllOptFactsThatCanBeModified();
    action.updateSuccessionCache(*this, currAction.first, tmpData.factsFromCondition);
    actionTmpData.emplace(currAction.first, std::move(tmpData));
  }

  // Add successions cache of the events
  for (auto& currSetOfEvents : _setOfEvents)
  {
    const auto& currSetOfEventsId = currSetOfEvents.first;
    for (auto& currEvent : currSetOfEvents.second.events())
    {
      currEvent.second.updateSuccessionCache(*this, currSetOfEventsId, currEvent.first);
      auto fullEventId = generateFullEventId(currSetOfEventsId, currEvent.first);
      eventTmpData.emplace(fullEventId, EventWithTmpData(currSetOfEventsId, currEvent.first, currEvent.second));
    }
  }

  // Add successions without interest cache (and update successions cache of the actions)
  for (auto& currAction : actionTmpData)
  {
    ActionWithConditionAndFactFacts& tmpData = currAction.second;
    tmpData.action.actionsSuccessionsWithoutInterestCache.clear();

    for (auto& currActionSucc : actionTmpData)
    {
      if (tmpData.isImpossibleSuccession(currActionSucc.second) ||
          !tmpData.doesSuccessionsHasAnInterest(currActionSucc.second))
      {
        tmpData.action.actionsSuccessionsWithoutInterestCache.insert(currActionSucc.second.actionId);
        tmpData.action.removePossibleSuccessionCache(currActionSucc.second.actionId);
      }
    }
  }


  for (auto& currAction : actionTmpData)
  {
    ActionWithConditionAndFactFacts& tmpData = currAction.second;
    Successions successions;
    if (tmpData.action.effect.worldStateModification)
      tmpData.action.effect.worldStateModification->getSuccesions(successions);
    if (tmpData.action.effect.potentialWorldStateModification)
      tmpData.action.effect.potentialWorldStateModification->getSuccesions(successions);

    for (const auto& currFollowingActionId : successions.actions)
    {
      auto itFollowingAction = actionTmpData.find(currFollowingActionId);
      if (itFollowingAction == actionTmpData.end())
        throw std::runtime_error("Following action id not found: " + currFollowingActionId + ".");
      itFollowingAction->second.invertSuccessionsFromActions.insert(currAction.first);
    }

    for (const auto& currIdToEvents : successions.events)
    {
      for (const auto& currFollowingEventId : currIdToEvents.second)
      {
        auto fullEventId = generateFullEventId(currIdToEvents.first, currFollowingEventId);
        auto itFollowingEvent = eventTmpData.find(fullEventId);
        if (itFollowingEvent == eventTmpData.end())
          throw std::runtime_error("Following event id not found: " + fullEventId + ".");
        itFollowingEvent->second.invertSuccessionsFromActions.insert(currAction.first);
      }
    }
  }

  for (auto& currEvent : eventTmpData)
  {
    EventWithTmpData& tmpData = currEvent.second;
    Successions successions;
    if (tmpData.event.factsToModify)
      tmpData.event.factsToModify->getSuccesions(successions);

    for (const auto& currFollowingActionId : successions.actions)
    {
      auto itFollowingAction = actionTmpData.find(currFollowingActionId);
      if (itFollowingAction == actionTmpData.end())
        throw std::runtime_error("Following action id not found: " + currFollowingActionId + ".");
      itFollowingAction->second.invertSuccessionsFromEvents.insert(currEvent.first);
    }

    for (const auto& currIdToEvents : successions.events)
    {
      for (const auto& currFollowingEventId : currIdToEvents.second)
      {
        auto fullEventId = generateFullEventId(currIdToEvents.first, currFollowingEventId);
        auto itFollowingEvent = eventTmpData.find(fullEventId);
        if (itFollowingEvent == eventTmpData.end())
          throw std::runtime_error("Following event id not found: " + fullEventId + ".");
        itFollowingEvent->second.invertSuccessionsFromEvents.insert(currEvent.first);
      }
    }
  }


  for (auto& currAction : actionTmpData)
  {
    ActionWithConditionAndFactFacts& tmpData = currAction.second;
    tmpData.action.actionsPredecessorsCache.clear();
    tmpData.action.eventsPredecessorsCache.clear();
    _updateActionsPredecessors(tmpData.action.actionsPredecessorsCache,
                               tmpData.action.eventsPredecessorsCache,
                               tmpData.invertSuccessionsFromActions,
                               tmpData.invertSuccessionsFromEvents,
                               actionTmpData, eventTmpData);
  }

  for (auto& currEvent : eventTmpData)
  {
    EventWithTmpData& tmpData = currEvent.second;
    tmpData.event.actionsPredecessorsCache.clear();
    tmpData.event.eventsPredecessorsCache.clear();
    _updateActionsPredecessors(tmpData.event.actionsPredecessorsCache,
                               tmpData.event.eventsPredecessorsCache,
                               tmpData.invertSuccessionsFromActions,
                               tmpData.invertSuccessionsFromEvents,
                               actionTmpData, eventTmpData);
  }

}


const std::string& Domain::getSetOfEventsIdFromConstructor()
{
  static const std::string setOfEventsIdFromConstructor = "soe_from_constructor";
  return setOfEventsIdFromConstructor;
}



} // !ogp
