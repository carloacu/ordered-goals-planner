#include <orderedgoalsplanner/types/setofevents.hpp>
#include <orderedgoalsplanner/util/util.hpp>

namespace ogp
{

SetOfEvents::SetOfEvents(const Event& pEvent)
  : _events(),
    _reachableEventLinks()
{
  add(pEvent);
}


EventId SetOfEvents::add(const Event& pEvent,
                         const EventId& pEventId)
{
  auto isIdOkForInsertion = [this](const std::string& pId)
  {
    return _events.count(pId) == 0;
  };
  auto newId = incrementLastNumberUntilAConditionIsSatisfied(pEventId, isIdOkForInsertion);
  _events.emplace(newId, pEvent);
  if (pEvent.precondition)
    _reachableEventLinks.add(*pEvent.precondition, newId);
  return newId;
}


} // !ogp
