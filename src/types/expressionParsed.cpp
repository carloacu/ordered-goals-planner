#include "expressionParsed.hpp"
#include <stdexcept>
#include <orderedgoalsplanner/types/factoptional.hpp>
#include <orderedgoalsplanner/types/ontology.hpp>
#include <orderedgoalsplanner/util/util.hpp>

namespace ogp
{

namespace
{

bool _isASeparatorForTheBeginOfAFollowingExpression(char pChar)
{
  return pChar == '&' || pChar == '|' || pChar == '+' || pChar == '-' || pChar == '<' || pChar == '>';
}

bool _isASeparator(char pChar)
{
  return pChar == ' ' || pChar == '(' || pChar == ')' || pChar == ',' || pChar == '=' || pChar == '!' ||
      _isASeparatorForTheBeginOfAFollowingExpression(pChar);
}

}


ExpressionParsed ExpressionParsed::clone() const
{
  ExpressionParsed res;
  res.name = name;
  for (const auto& arg : arguments)
     res.arguments.emplace_back(arg.clone());
  res.value = value;
  res.isValueNegated = isValueNegated;
  res.isAFunction = isAFunction;
  res.separatorToFollowingExp = separatorToFollowingExp;
  if (followingExpression)
    res.followingExpression = std::make_unique<ExpressionParsed>(followingExpression->clone());
  return res;
}

std::string ExpressionParsed::toStr() const
{
  std::string res = name + "(";
  bool firstIteration = true;
  for (const auto& currArg : arguments)
  {
    if (firstIteration)
      res += ", ";
    else
      firstIteration = false;
    res += currArg.toStr();
  }
  return res + ")";
}


FactOptional ExpressionParsed::toFact(const Ontology& pOntology,
                                      const SetOfEntities& pObjects,
                                      const std::vector<Parameter>& pParameters,
                                      bool pIsOkIfValueIsMissing,
                                      const std::map<std::string, Entity>* pParameterNamesToEntityPtr) const
{
  std::vector<std::string> argumentStrs;
  for (auto& currArg : arguments)
    argumentStrs.emplace_back(currArg.name);

  std::string factName;
  bool isFactNegated = false;
  if (name.empty())
    throw std::runtime_error("Fact cannot have an empty name");
  if (name[0] == '!')
  {
    isFactNegated = true;
    factName = name.substr(1, name.size() - 1);
  }
  else if (name == "=" && arguments.size() == 2)
  {
    auto it = arguments.begin();
    auto newFactOpt = it->toFact(pOntology, pObjects, pParameters, true, pParameterNamesToEntityPtr);
    ++it;
    if (it->name == Fact::getUndefinedEntity().value)
    {
      newFactOpt.isFactNegated = !newFactOpt.isFactNegated;
      newFactOpt.fact.setValue(Entity::createAnyEntity());
    }
    else
    {
      newFactOpt.fact.setValue(Entity::fromUsage(it->name, pOntology, pObjects, pParameters, pParameterNamesToEntityPtr));
    }

    return newFactOpt;
  }
  else if (name == "not" && arguments.size() == 1)
  {
    auto it = arguments.begin();
    auto newFactOpt = it->toFact(pOntology, pObjects, pParameters, pIsOkIfValueIsMissing, pParameterNamesToEntityPtr);
    newFactOpt.isFactNegated = !newFactOpt.isFactNegated;
    return newFactOpt;
  }
  else
  {
    factName = name;
  }

  return FactOptional(isFactNegated, factName, argumentStrs, value, isValueNegated, pOntology, pObjects, pParameters, pIsOkIfValueIsMissing, pParameterNamesToEntityPtr);
}



void ExpressionParsed::extractMissingObjects(std::list<Entity>& pRes,
                                             const Ontology& pOntology,
                                             const SetOfEntities& pObjects) const
{
  auto* predicatePtr = pOntology.nameToPredicatePtr(name);
  if (predicatePtr == nullptr)
    throw std::runtime_error("Predicate name or a derived predicate name \"" + name + "\" not found");
  auto& predicate = *predicatePtr;

  auto itParam = predicate.parameters.begin();
  for (const auto& currArg : arguments)
  {
    if (itParam == predicate.parameters.end())
      throw std::runtime_error("Predicate usage of \"" + name + "\" has too maany arguments");
    if (!Entity::isParamOrDeclaredEntity(currArg.name, pOntology, pObjects))
      pRes.emplace_back(Entity(currArg.name, itParam->type));
    ++itParam;
  }

  if (value != "" &&
      !Entity::isParamOrDeclaredEntity(value, pOntology, pObjects))
  {
    if (!predicate.value)
      throw std::runtime_error("Predicate usage of \"" + name + "\" has a value but must ont have one");
    pRes.emplace_back(Entity(value, predicate.value));
  }
}


void ExpressionParsed::extractParameters(std::list<Parameter>& pRes,
                                         const Ontology& pOntology) const
{
  auto* predicatePtr = pOntology.nameToPredicatePtr(name);
  if (predicatePtr == nullptr)
    throw std::runtime_error("Predicate name or a derived predicate name \"" + name + "\" not found");
  auto& predicate = *predicatePtr;

  auto itParam = predicate.parameters.begin();
  for (const auto& currArg : arguments)
  {
    if (itParam == predicate.parameters.end())
      throw std::runtime_error("Predicate usage of \"" + name + "\" has too maany arguments");
    if (Entity::isParam(currArg.name))
      pRes.emplace_back(Parameter(currArg.name, itParam->type));
    ++itParam;
  }

  if (value != "" &&
      Entity::isParam(value))
  {
    if (!predicate.value)
      throw std::runtime_error("Predicate usage of \"" + name + "\" has a value but must ont have one");
    pRes.emplace_back(Parameter(value, predicate.value));
  }
}


ExpressionParsed ExpressionParsed::fromStr(const std::string& pStr,
                                           std::size_t& pPos)
{
  ExpressionParsed res;
  auto strSize = pStr.size();
  std::size_t beginOfNamePos = pPos;

  // extract name
  while (pPos < strSize)
  {
    if (beginOfNamePos == pPos)
    {
      if (pStr[pPos] == ' ')
      {
        ++pPos;
        beginOfNamePos = pPos;
        continue;
      }
    }
    else
    {
      if (_isASeparator(pStr[pPos]))
      {
        res.name = pStr.substr(beginOfNamePos, pPos - beginOfNamePos);
        break;
      }
    }
    ++pPos;
  }

  if (res.name.empty())
  {
    if (beginOfNamePos == pPos)
      throw std::runtime_error("Predicate is missing in expression: \"" + pStr + "\"");
    res.name = pStr.substr(beginOfNamePos, pPos - beginOfNamePos);
  }

  // extract arguments
  if (pPos < strSize)
  {
    if (pStr[pPos] == '(')
    {
      res.isAFunction = true;
      if (pPos + 1 < strSize && pStr[pPos + 1] != ')')
      {
        do
        {
          ++pPos;
          res.arguments.emplace_back(fromStr(pStr, pPos));
        }
        while (pStr[pPos] == ',');
      }
      else
      {
        ++pPos;
      }

      if (pStr[pPos] == ')')
        ++pPos;
      else
        throw std::runtime_error("Arguments parenthesis is not closed: \"" + pStr + "\"");
    }
  }

  // extract value
  if (pPos < strSize)
  {
    if (pStr[pPos] == '!')
    {
      res.isValueNegated = true;
      ++pPos;
    }
    if (pStr[pPos] == '=')
    {
      res.isAFunction = true;
      ++pPos;
      std::size_t beginOfValuePos = pPos;

      while (pPos < strSize)
      {
        if (_isASeparator(pStr[pPos]))
        {
          res.value = pStr.substr(beginOfValuePos, pPos - beginOfValuePos);
          break;
        }
        ++pPos;
      }

      if (res.value.empty() && pPos > beginOfValuePos)
        res.value = pStr.substr(beginOfValuePos, pPos - beginOfValuePos);
    }
  }

  // extract following expression
  while (pPos < strSize)
  {
    if (pStr[pPos] == ' ')
    {
      ++pPos;
      continue;
    }

    if (_isASeparatorForTheBeginOfAFollowingExpression(pStr[pPos]))
    {
      res.separatorToFollowingExp = pStr[pPos];
      ++pPos;
      res.followingExpression = std::make_unique<ExpressionParsed>(fromStr(pStr, pPos));
    }

    return res;
  }

  return res;
}


ExpressionParsed ExpressionParsed::fromPddl(const std::string& pStr,
                                            std::size_t& pPos,
                                            bool pCanHaveFollowingExpression)
{
  ExpressionParsed res;
  auto strSize = pStr.size();
  res.skipSpacesWithTagExtraction(pStr, pPos);
  if (pPos >= strSize)
    return res;

  if (pStr[pPos] == '(')
  {
    ++pPos;
    res.isAFunction = true;
    res.skipSpacesWithTagExtraction(pStr, pPos);
    std::size_t beginOfTokenPos = pPos;

    bool inName = true;
    while (pPos < strSize)
    {
      if (pStr[pPos] == ';')
      {
        res.skipSpacesWithTagExtraction(pStr, pPos);
        beginOfTokenPos = pPos;
      }

      if (!inName || isEndOfTokenSeparator(pStr[pPos]))
      {
        if (inName)
        {
          res.name = pStr.substr(beginOfTokenPos, pPos - beginOfTokenPos);
          if (res.name.empty())
          {
            res = fromPddl(pStr, pPos, pCanHaveFollowingExpression);
            return res;
          }
          res.skipSpacesWithTagExtraction(pStr, pPos);
          inName = false;
          continue;
        }

        if (pStr[pPos] == ')')
        {
          ++pPos;
          break;
        }

        auto prePos = pPos;
        res.arguments.emplace_back(fromPddl(pStr, pPos, pCanHaveFollowingExpression));
        if (pPos > prePos)
          continue;
      }
      ++pPos;
    }

  }
  else
  {
    res.name = parseToken(pStr, pPos);
  }

  // extract following expression
  if (pCanHaveFollowingExpression)
  {
    while (pPos < strSize)
    {
      if (pStr[pPos] == ' ')
      {
        ++pPos;
        continue;
      }

      if (_isASeparatorForTheBeginOfAFollowingExpression(pStr[pPos]))
      {
        res.separatorToFollowingExp = pStr[pPos];
        ++pPos;
        res.followingExpression = std::make_unique<ExpressionParsed>(fromPddl(pStr, pPos, pCanHaveFollowingExpression));
      }

      break;
    }
  }

  res.skipSpacesWithTagExtraction(pStr, pPos);
  return res;
}

void ExpressionParsed::skipSpaces(const std::string& pStr,
                                  std::size_t& pPos)
{
  auto strSize = pStr.size();
  while (pPos < strSize)
  {
    if (pStr[pPos] == ';')
       ExpressionParsed::moveUntilEndOfLine(pStr, pPos);
    else if (pStr[pPos] != ' ' && pStr[pPos] != '\n' && pStr[pPos] != '\t')
      break;
    ++pPos;
  }
}

void ExpressionParsed::skipSpacesWithTagExtraction(const std::string& pStr,
                                                   std::size_t& pPos)
{
  auto strSize = pStr.size();
  while (pPos < strSize)
  {
    if (pStr[pPos] == ';')
       moveUntilEndOfLineWithTagExtraction(pStr, pPos);
    else if (pStr[pPos] != ' ' && pStr[pPos] != '\n' && pStr[pPos] != '\t')
      break;
    ++pPos;
  }
}


void ExpressionParsed::moveUntilEndOfLine(const std::string& pStr,
                                          std::size_t& pPos)
{
  auto strSize = pStr.size();
  while (pPos < strSize)
  {
    if (pStr[pPos] == '\n')
      break;
    ++pPos;
  }
}


void ExpressionParsed::moveUntilEndOfLineWithTagExtraction(const std::string& pStr,
                                                           std::size_t& pPos)
{
  auto strSize = pStr.size();
  std::optional<std::size_t> beginPos;
  std::optional<std::size_t> endPos;
  while (pPos < strSize)
  {
    if (pStr[pPos] == '_' && pPos + 1 < strSize && pStr[pPos + 1] == '_')
    {
      beginPos.emplace(pPos);
      endPos.reset();
    }
    else if (pStr[pPos] == ' ')
    {
      if (beginPos)
      {
        tags.insert(pStr.substr(*beginPos, pPos - *beginPos));
        beginPos.reset();
      }
    }
    else if (pStr[pPos] == '\n')
    {
      if (beginPos)
      {
        tags.insert(pStr.substr(*beginPos, pPos - *beginPos));
        beginPos.reset();
      }
      break;
    }
    ++pPos;
  }

  if (beginPos)
    tags.insert(pStr.substr(*beginPos, strSize - *beginPos));
}


void ExpressionParsed::moveUntilClosingParenthesis(const std::string& pStr,
                                                   std::size_t& pPos)
{
  auto strSize = pStr.size();
  while (pPos < strSize)
  {
    if (pStr[pPos] == ')')
      break;
    ++pPos;
  }
}


std::string ExpressionParsed::parseToken(const std::string& pStr,
                                         std::size_t& pPos)
{
  std::size_t beginOfTokenPos = pPos;
  auto res = parseTokenThatCanBeEmpty(pStr, pPos);
  if (res.empty())
  {
    auto strSize = pStr.size();
    throw std::runtime_error("Empty token in str " + pStr.substr(beginOfTokenPos, strSize - beginOfTokenPos));
  }
  return res;
}


std::string ExpressionParsed::parseTokenThatCanBeEmpty(const std::string& pStr,
                                                       std::size_t& pPos)
{
  auto strSize = pStr.size();
  skipSpaces(pStr, pPos);
  std::string res = "";
  std::size_t beginOfTokenPos = pPos;
  while (pPos < strSize)
  {
    if (pStr[pPos] == ';')
    {
      res = pStr.substr(beginOfTokenPos, pPos - beginOfTokenPos);
      ExpressionParsed::moveUntilEndOfLine(pStr, pPos);
      ++pPos;
      return res;
    }

    if (isEndOfTokenSeparator(pStr[pPos]))
    {
      res = pStr.substr(beginOfTokenPos, pPos - beginOfTokenPos);
      break;
    }
    ++pPos;
  }
  if (res.empty())
  {
    res = pStr.substr(beginOfTokenPos, pPos - beginOfTokenPos);
  }
  return res;
}


bool ExpressionParsed::isEndOfTokenSeparator(char pChar)
{
  return pChar == ' ' || pChar == '\n' || pChar == ')' || pChar == '(';
}


} // !ogp
