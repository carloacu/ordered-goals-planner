#ifndef INCLUDE_ORDEREDGOALSPLANNER_ALIAS_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_ALIAS_HPP

#include <string>


namespace ogp
{
using ActionId = std::string;
using SetOfEventsId = std::string;
using EventId = std::string;
using CallbackId = std::string;

using FullEventId = std::string;


inline const FullEventId generateFullEventId(const SetOfEventsId& pSetOfEventsId, const EventId& pEventId)
{
  return pSetOfEventsId + "|" + pEventId;
}


} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_ALIAS_HPP
