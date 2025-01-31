#ifndef INCLUDE_ORDEREDGOALSPLANNER_TYPES_ACTIONINVOCATION_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_TYPES_ACTIONINVOCATION_HPP

#include <map>
#include <set>
#include <string>
#include "../util/api.hpp"
#include <orderedgoalsplanner/types/entitieswithparamconstraints.hpp>

namespace ogp
{
struct Domain;
struct Entity;
struct Parameter;


/// Struct gathering the identifier of an action with some parameter values.
struct ORDEREDGOALSPLANNER_API ActionInvocation
{
  /// Construct ActionInstance.
  ActionInvocation(const std::string& pActionId,
                   const std::map<Parameter, Entity>& pParameters);
  ActionInvocation(const std::string& pActionId,
                   const ParameterValuesWithConstraints& pParameters);
  ~ActionInvocation();

  /**
   * @brief Serialize in a string.
   * @return String containing the action identifier with his parameters.
   */
  std::string toStr() const;

  std::string toPddl(const Domain& pDomain) const;

  /// Identifer of the action.
  std::string actionId;
  /// Action parameter values.
  std::map<Parameter, Entity> parameters;
};


} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_TYPES_ACTIONINVOCATION_HPP
