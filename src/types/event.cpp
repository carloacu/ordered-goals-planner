#include <orderedgoalsplanner/types/event.hpp>

namespace ogp
{


Event::Event(std::unique_ptr<Condition> pPrecondition,
             std::unique_ptr<WorldStateModification> pFactsToModify,
             const std::vector<Parameter>& pParameters,
             const std::map<int, std::vector<ogp::Goal>>& pGoalsToAdd)
  : parameters(pParameters),
    precondition(pPrecondition ? std::move(pPrecondition) : std::unique_ptr<Condition>()),
    factsToModify(pFactsToModify ? std::move(pFactsToModify) : std::unique_ptr<WorldStateModification>()),
    goalsToAdd(pGoalsToAdd),
    actionsPredecessorsCache(),
    eventsPredecessorsCache()
{
  assert(precondition);
  assert(factsToModify || !goalsToAdd.empty());
}


void Event::updateSuccessionCache(const Domain& pDomain,
                                  const SetOfEventsId& pSetOfEventsIdOfThisEvent,
                                  const EventId& pEventIdOfThisEvent)
{
  WorldStateModificationContainerId containerId;
  containerId.setOfEventsIdToExclude.emplace(pSetOfEventsIdOfThisEvent);
  containerId.eventIdToExclude.emplace(pEventIdOfThisEvent);

  auto optionalFactsToIgnore = precondition ? precondition->getAllOptFacts() : std::set<FactOptionalAndValueModification>();
  if (factsToModify)
    factsToModify->updateSuccesions(pDomain, containerId, optionalFactsToIgnore);
}

std::string Event::printSuccessionCache() const
{
  std::string res;
  if (factsToModify)
    factsToModify->printSuccesions(res);
  return res;
}


} // !ogp
