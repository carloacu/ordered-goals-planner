#ifndef INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFEVENTS_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFEVENTS_HPP

#include <map>
#include "../util/api.hpp"
#include <orderedgoalsplanner/types/event.hpp>
#include <orderedgoalsplanner/types/factoptionalstoid.hpp>
#include <orderedgoalsplanner/util/alias.hpp>

namespace ogp
{

/// Container of a set of events.
struct ORDEREDGOALSPLANNER_API SetOfEvents
{
  /// Construct the set of events.
  SetOfEvents() = default;

  SetOfEvents(const Event& pEvent);

  /**
   * @brief Add an event to check when the facts or the goals change.
   * @param pEventId Identifier of the event to add.
   * @param pEvent event to add.
   */
  EventId add(const Event& pEvent,
              const EventId& pEventId = "event");


  bool empty() const { return _events.empty(); }
  /// All events of the problem.
  const std::map<EventId, Event>& events() const { return _events; }
  std::map<EventId, Event>& events() { return _events; }
  /// Reachable event links.
  const FactOptionalsToId& reachableEventLinks() const { return _reachableEventLinks; }


private:
  /// Map of event indentifers to event.
  std::map<EventId, Event> _events{};
  /// Reachable event links.
  FactOptionalsToId _reachableEventLinks{};
};

} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFEVENTS_HPP
