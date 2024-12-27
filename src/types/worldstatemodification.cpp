#include <orderedgoalsplanner/types/worldstatemodification.hpp>
#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/types/ontology.hpp>
#include <orderedgoalsplanner/types/worldstate.hpp>
#include "expressionParsed.hpp"
#include <orderedgoalsplanner/util/util.hpp>
#include "worldstatemodificationprivate.hpp"

namespace ogp
{

void Successions::add(const Successions& pSuccessions)
{
  actions.insert(pSuccessions.actions.begin(), pSuccessions.actions.end());
  events.insert(pSuccessions.events.begin(), pSuccessions.events.end());
}


void Successions::addSuccesionsOptFact(const FactOptional& pFactOptional,
                                       const Domain& pDomain,
                                       const WorldStateModificationContainerId& pContainerId,
                                       const std::set<FactOptional>& pOptionalFactsToIgnore)
{
  if ((pFactOptional.fact.value() && pFactOptional.fact.value()->isAnyEntity()) ||
      pOptionalFactsToIgnore.count(pFactOptional) == 0)
  {
    const auto& preconditionToActionIds = pDomain.preconditionToActionIds();
    preconditionToActionIds.find([&](const std::string& pId) {
      if (!pContainerId.isAction(pId))
        actions.insert(pId);
      return ContinueOrBreak::CONTINUE;
    }, pFactOptional);

    auto& setOfEvents = pDomain.getSetOfEvents();
    for (auto& currSetOfEvents : setOfEvents)
    {
      std::set<EventId>* eventsPtr = nullptr;

      const FactOptionalsToId& conditionToEventIds = currSetOfEvents.second.reachableEventLinks();
      conditionToEventIds.find([&](const std::string& pId) {
        if (!pContainerId.isEvent(currSetOfEvents.first, pId))
        {
          if (eventsPtr == nullptr)
            eventsPtr = &events[currSetOfEvents.first];
          eventsPtr->insert(pId);
        }
        return ContinueOrBreak::CONTINUE;
      }, pFactOptional);

      /*
      auto& conditionToReachableEvents = !pFactOptional.isFactNegated ?
            currSetOfEvents.second.reachableEventLinks().conditionToEvents :
            currSetOfEvents.second.reachableEventLinks().notConditionToEvents;

      auto eventsFromCondtion = conditionToReachableEvents.find(pFactOptional.fact);
      for (const auto& currEvent : eventsFromCondtion)
      {
        if (!pContainerId.isEvent(currSetOfEvents.first, currEvent.id))
        {
          if (eventsPtr == nullptr)
            eventsPtr = &events[currSetOfEvents.first];
          eventsPtr->insert(currEvent.id);
        }
      }
      */
    }
  }
}

void Successions::print(std::string& pRes,
                        const FactOptional& pFactOptional) const
{
  if (empty())
    return;
  if (pRes != "")
    pRes += "\n";

  pRes += "fact: " + pFactOptional.toStr() + "\n";
  for (const auto& currActionId : actions)
    pRes += "action: " + currActionId + "\n";
  for (const auto& currEventSet : events)
    for (const auto& currEventId : currEventSet.second)
      pRes += "event: " + currEventSet.first + "|" + currEventId + "\n";
}


std::unique_ptr<WorldStateModification> WorldStateModification::createByConcatenation(const WorldStateModification& pWsModif1,
                                                                                      const WorldStateModification& pWsModif2)
{
  return std::make_unique<WorldStateModificationNode>(WorldStateModificationNodeType::AND,
                                                      pWsModif1.clone(nullptr),
                                                      pWsModif2.clone(nullptr));
}


} // !ogp
