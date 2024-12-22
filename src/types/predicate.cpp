#include <orderedgoalsplanner/types/predicate.hpp>
#include <stdexcept>
#include <orderedgoalsplanner/types/setoftypes.hpp>
#include "expressionParsed.hpp"


namespace ogp
{
namespace
{
void _parametersToStr(std::string& pStr,
                      const std::vector<Parameter>& pParameters,
                      const std::string& pSeparator)
{
  bool firstIteration = true;
  for (auto& param : pParameters)
  {
    if (firstIteration)
      firstIteration = false;
    else
      pStr += pSeparator;
    pStr += param.toStr();
  }
}

}

Predicate::Predicate(const std::string& pStr,
                     bool pStrPddlFormated,
                     const SetOfTypes& pSetOfTypes,
                     std::size_t pBeginPos,
                     std::size_t* pResPos)
  : name(),
    parameters(),
    value()
{
  std::size_t pos = pBeginPos;
  auto expressionParsed = pStrPddlFormated ?
        ExpressionParsed::fromPddl(pStr, pos, true) :
        ExpressionParsed::fromStr(pStr, pos);

  name = expressionParsed.name;
  for (auto& currArg : expressionParsed.arguments)
    if (currArg.followingExpression)
    {
      auto type = pSetOfTypes.nameToType(currArg.followingExpression->name);
      parameters.emplace_back(Parameter(currArg.name, type));
    }
  if (expressionParsed.followingExpression) {
    value = pSetOfTypes.nameToType(expressionParsed.followingExpression->name);
  }

  if (pResPos != nullptr)
  {
    if (pos <= pBeginPos)
      throw std::runtime_error("Failed to parse a predicate in str " + pStr.substr(pBeginPos, pStr.size() - pBeginPos));
    *pResPos = pos;
  }
}


std::string Predicate::toPddl() const
{
  std::string res = "(" + name;
  if (!parameters.empty())
  {
    res += " ";
    _parametersToStr(res, parameters, " ");
  }
  res += ")";
  if (value)
    res += " - " + value->name;
  return res;
}

std::string Predicate::toStr() const
 {
   auto res = name + "(";
   _parametersToStr(res, parameters, ", ");
   res += ")";
   if (value)
     res += " - " + value->name;
   return res;
 }


 bool Predicate::operator==(const Predicate& pOther) const
 {
   if (name == pOther.name && parameters == pOther.parameters)
   {
     if (!value && !pOther.value)
       return true;
     if (value && pOther.value && value->name == pOther.value->name)
       return true;
   }
   return false;
 }


} // !ogp
