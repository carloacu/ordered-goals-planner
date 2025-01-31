#include <orderedgoalsplanner/types/actioninvocationwithgoal.hpp>


namespace ogp
{

ActionInvocationWithGoal::ActionInvocationWithGoal(const std::string& pActionId,
                                                   const std::map<Parameter, Entity>& pParameters,
                                                   std::unique_ptr<Goal> pFromGoal,
                                                   int pFromGoalPriority)
  : actionInvocation(pActionId, pParameters),
    fromGoal(std::move(pFromGoal)),
    fromGoalPriority(pFromGoalPriority)
{
}


ActionInvocationWithGoal::ActionInvocationWithGoal(const std::string& pActionId,
                                                   const ParameterValuesWithConstraints& pParameters,
                                                   std::unique_ptr<Goal> pFromGoal,
                                                   int pFromGoalPriority)
  : actionInvocation(pActionId, pParameters),
    fromGoal(std::move(pFromGoal)),
    fromGoalPriority(pFromGoalPriority)
{
}


ActionInvocationWithGoal::ActionInvocationWithGoal
(const ActionInvocationWithGoal& pOther)
 : actionInvocation(pOther.actionInvocation),
   fromGoal(pOther.fromGoal ? pOther.fromGoal->clone() : std::unique_ptr<Goal>()),
   fromGoalPriority(pOther.fromGoalPriority)
{
}


void ActionInvocationWithGoal::operator=(const ActionInvocationWithGoal& pOther)
{
  actionInvocation = pOther.actionInvocation;
  fromGoal = pOther.fromGoal ? pOther.fromGoal->clone() : std::unique_ptr<Goal>();
  fromGoalPriority = pOther.fromGoalPriority;
}


} // !ogp
