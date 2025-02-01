#ifndef INCLUDE_ORDEREDGOALSPLANNER_ENTITIESWITHPARAMCONSTRANTS_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_ENTITIESWITHPARAMCONSTRANTS_HPP

#include <map>
#include <set>
#include <orderedgoalsplanner/types/entity.hpp>
#include <orderedgoalsplanner/types/parameter.hpp>

namespace ogp
{

/// Parameters to entities possibilities
using ParamToEntityValues = std::map<Parameter, std::set<Entity>>;

/// Entity possibilities to constraints on other parameters values
using EntitiesWithParamConstaints = std::map<Entity, ParamToEntityValues>;

/// Parameters to entities possibilities (with constraints on other parameters values)
using ParameterValuesWithConstraints = std::map<Parameter, EntitiesWithParamConstaints>;

} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_ENTITIESWITHPARAMCONSTRANTS_HPP
