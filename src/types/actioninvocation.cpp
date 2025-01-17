#include <orderedgoalsplanner/types/actioninvocation.hpp>
#include <orderedgoalsplanner/types/domain.hpp>
#include <orderedgoalsplanner/types/entity.hpp>
#include <orderedgoalsplanner/types/parameter.hpp>

namespace ogp
{

ActionInvocation::ActionInvocation(const std::string& pActionId,
                                   const std::map<Parameter, Entity>& pParameters)
  : actionId(pActionId),
    parameters(pParameters)
{
}


ActionInvocation::ActionInvocation(const std::string& pActionId,
                                   const std::map<Parameter, std::set<Entity>>& pParameters)
  : actionId(pActionId),
    parameters()
{
  for (auto& currParam : pParameters)
    if (!currParam.second.empty())
      parameters.emplace(currParam.first, *currParam.second.begin());
}


ActionInvocation::~ActionInvocation() {}


std::string ActionInvocation::toStr() const
{
  std::string res = actionId;
  if (!parameters.empty())
  {
    res += "(";
    bool firstIeration = true;
    for (const auto& currParam : parameters)
    {
      if (firstIeration)
        firstIeration = false;
      else
        res += ", ";
      res += currParam.first.name + " -> " + currParam.second.value;
    }
    res += ")";
  }
  return res;
}


std::string ActionInvocation::toPddl(const Domain& pDomain) const
{
  std::stringstream ss;
  auto* actionPtr = pDomain.getActionPtr(actionId);
  if (actionPtr == nullptr)
    throw std::runtime_error("Action " + actionId + " from a plan is not found in the domain");
  auto& action = *actionPtr;

  ss << "(" << actionId;
  if (!action.parameters.empty())
  {
    for (const auto& currParam : action.parameters)
    {
      auto itParamToValues = parameters.find(currParam);
      if (itParamToValues == parameters.end())
        throw std::runtime_error("Parameter in action not found in action invocation");
      ss << " " + itParamToValues->second.value;
    }
  }
  ss << ") [" << numberToString(action.duration) << "]";
  return ss.str();
}


} // !ogp
