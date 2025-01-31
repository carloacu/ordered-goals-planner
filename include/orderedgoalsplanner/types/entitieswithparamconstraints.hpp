#ifndef INCLUDE_ORDEREDGOALSPLANNER_ENTITIESWITHPARAMCONSTRANTS_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_ENTITIESWITHPARAMCONSTRANTS_HPP

#include <map>
#include <set>
#include <orderedgoalsplanner/types/entity.hpp>
#include <orderedgoalsplanner/types/parameter.hpp>

namespace ogp
{

using ParamConstaints = std::map<Parameter, std::set<Entity>>;
using EntitiesWithParamConstaints = std::map<Entity, ParamConstaints>;
using ParameterValuesWithConstraints = std::map<Parameter, EntitiesWithParamConstaints>;

} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_ENTITIESWITHPARAMCONSTRANTS_HPP
