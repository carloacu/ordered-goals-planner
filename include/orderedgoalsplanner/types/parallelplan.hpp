#ifndef INCLUDE_ORDEREDGOALSPLANNER_TYPES_PARALLELPLAN_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_TYPES_PARALLELPLAN_HPP

#include <list>
#include "../util/api.hpp"
#include <orderedgoalsplanner/types/actionstodoinparallel.hpp>


namespace ogp
{

struct ORDEREDGOALSPLANNER_API ParallelPan
{
  /// Ordered list of actions set to do in parallel
  std::list<ActionsToDoInParallel> actionsToDoInParallel;

  std::list<std::string> extractSatisiedGoals() const;

  std::size_t cost() const;
};


} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_TYPES_PARALLELPLAN_HPP
