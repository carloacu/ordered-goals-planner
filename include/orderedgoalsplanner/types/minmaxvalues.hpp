#ifndef INCLUDE_ORDEREDGOALSPLANNER_TYPES_MINMAXVALUES_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_TYPES_MINMAXVALUES_HPP

#include <optional>
#include <orderedgoalsplanner/util/util.hpp>

namespace ogp
{

struct MinMaxValues
{
  std::optional<Number> min;
  std::optional<Number> max;
};


} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_TYPES_MINMAXVALUES_HPP
