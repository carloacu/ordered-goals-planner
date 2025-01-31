#ifndef INCLUDE_ORDEREDGOALSPLANNER_TYPES_ACTIONINVOCATIONWITHGOAL_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_TYPES_ACTIONINVOCATIONWITHGOAL_HPP

#include <memory>
#include <string>
#include "../util/api.hpp"
#include <orderedgoalsplanner/types/goal.hpp>
#include <orderedgoalsplanner/types/actioninvocation.hpp>


namespace ogp
{

/// Struct gathering the result of one step of the planner.
struct ORDEREDGOALSPLANNER_API ActionInvocationWithGoal
{
  /// Construct ActionInvocationWithGoal.
  ActionInvocationWithGoal(const std::string& pActionId,
                           const std::map<Parameter, Entity>& pParameters,
                           std::unique_ptr<Goal> pFromGoal,
                           int pFromGoalPriority);
  ActionInvocationWithGoal(const std::string& pActionId,
                           const ParameterValuesWithConstraints& pParameters,
                           std::unique_ptr<Goal> pFromGoal,
                           int pFromGoalPriority);

  /// Construct a copy.
  ActionInvocationWithGoal(const ActionInvocationWithGoal& pOther);

  /// Copy operator.
  void operator=(const ActionInvocationWithGoal& pOther);

  /// Action with his parameters.
  ActionInvocation actionInvocation;
  /// Goal that motivated the action.
  std::unique_ptr<Goal> fromGoal;
  /// Priority of the goal that motivated the action.
  int fromGoalPriority;
};


} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_TYPES_ACTIONINVOCATIONWITHGOAL_HPP
