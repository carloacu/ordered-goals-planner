#include <orderedgoalsplanner/util/util.hpp>
#include <cctype> // For isdigit()
#include <sstream>
#include <orderedgoalsplanner/types/entity.hpp>
#include <orderedgoalsplanner/types/parameter.hpp>
#include <orderedgoalsplanner/types/setofentities.hpp>

namespace ogp
{
bool ORDEREDGOALSPLANNER_DEBUG_FOR_TESTS = false;  // Define the variable and initialize it

namespace
{

template <typename T>
T _lexical_cast(const std::string& pStr)
{
  bool firstChar = true;
  for (const auto& currChar : pStr)
  {
    if ((currChar < '0' || currChar > '9') &&
        !(firstChar && currChar == '-'))
      throw std::runtime_error("bad lexical cast: source type value could not be interpreted as target");
    firstChar = false;
  }
  return atoi(pStr.c_str());
}

void _typeToEntitiesRec(std::set<Entity>& pRes,
                        const Type& pParamType,
                        const SetOfEntities& pConstants,
                        const SetOfEntities& pObjects)
{
  auto* constantsPtr = pConstants.typeNameToEntities(pParamType.name);
  if (constantsPtr != nullptr)
  {
    if (pRes.empty())
      pRes = *constantsPtr;
    else
      pRes.insert(constantsPtr->begin(), constantsPtr->end());
  }

  auto* entitiesPtr = pObjects.typeNameToEntities(pParamType.name);
  if (entitiesPtr != nullptr)
  {
    if (pRes.empty())
      pRes = *entitiesPtr;
    else
      pRes.insert(entitiesPtr->begin(), entitiesPtr->end());
  }

  for (const auto& currSubType : pParamType.subTypes)
    _typeToEntitiesRec(pRes, *currSubType, pConstants, pObjects);
}

}


// Function to convert a string to either an int or a float and store it in a variant
Number stringToNumber(const std::string& str) {
    std::istringstream iss(str);

    // Try to parse as an int first
    int intValue;
    if (iss >> intValue && iss.eof()) {
        return intValue;  // Successfully parsed as an int
    }

    // Clear error flags and try parsing as a float
    iss.clear();
    iss.str(str);

    float floatValue;
    if (iss >> floatValue && iss.eof()) {
        return floatValue;  // Successfully parsed as a float
    }

    // If neither works, throw an exception
    throw std::invalid_argument("Invalid number format: " + str);
}

// Overloaded operator for addition of two Number objects
Number operator+(const Number& lhs, const Number& rhs) {
    return std::visit([](auto&& l, auto&& r) -> Number {
        return l + r;
    }, lhs, rhs);
}

// Overloaded operator for substraction of two Number objects
Number operator-(const Number& lhs, const Number& rhs) {
    return std::visit([](auto&& l, auto&& r) -> Number {
        return l - r;
    }, lhs, rhs);
}

// Overloaded operator for multiplication of two Number objects
Number operator*(const Number& lhs, const Number& rhs) {
    return std::visit([](auto&& l, auto&& r) -> Number {
        return l * r;
    }, lhs, rhs);
}

// Overloaded operator for equality comparison of two Number objects
bool operator==(const Number& lhs, const Number& rhs) {
    return std::visit([](auto&& l, auto&& r) -> bool {
        return static_cast<float>(l) == static_cast<float>(r);
    }, lhs, rhs);
}

// Overloaded operator for addition assignment of two Number objects
Number& operator+=(Number& lhs, const Number& rhs) {
    lhs = std::visit([](auto&& l, auto&& r) -> Number {
        return l + r;
    }, lhs, rhs);
    return lhs;
}

// Function to convert a Number to a std::string
std::string numberToString(const Number& num) {
    return std::visit([](auto&& value) -> std::string {
        return std::to_string(value);
    }, num);
}


bool isNumber(const std::string& str) {
    if (str.empty()) return false; // Empty strings are not numbers

    size_t i = 0;
    if (str[i] == '-' || str[i] == '+') {
        ++i; // Skip the sign character
    }

    bool hasDigits = false;
    bool hasDecimalPoint = false;

    for (; i < str.size(); ++i) {
        if (std::isdigit(str[i])) {
            hasDigits = true; // Found at least one digit
        } else if (str[i] == '.') {
            if (hasDecimalPoint) {
                return false; // Multiple decimal points are invalid
            }
            hasDecimalPoint = true;
        } else {
            return false; // Invalid character
        }
    }

    return hasDigits; // Valid if there's at least one digit
}

void _unfoldMapWithSet(std::list<std::map<Parameter, Entity>>& pOutMap,
                       const std::map<Parameter, std::set<Entity>>& pInMap,
                       std::map<Parameter, std::set<Entity>>::const_iterator pItInMap)
{
  auto nextIt = pItInMap;
  ++nextIt;
  if (nextIt == pInMap.end())
  {
    for (auto& currValue : pItInMap->second)
      pOutMap.emplace_back(std::map<Parameter, Entity>{{pItInMap->first, currValue}});
    return;
  }

  for (auto it = pItInMap; it != pInMap.end(); ++it)
  {
    auto key = it->first;
    auto values = std::move(it->second);
    ++it;

    std::list<std::map<Parameter, Entity>> subRes;
    _unfoldMapWithSet(subRes, pInMap, it);

    for (auto& currValue : values)
    {
      auto newRes = subRes;
      for (auto& currSubResValue : newRes)
      {
        currSubResValue.try_emplace(key, currValue);
        pOutMap.emplace_back(std::move(currSubResValue));
      }
    }
  }
}

void unfoldMapWithSet(std::list<std::map<Parameter, Entity>>& pOutMap,
                      const std::map<Parameter, std::set<Entity>>& pInMap)
{
  if (pInMap.empty())
    return;
  _unfoldMapWithSet(pOutMap, pInMap, pInMap.begin());
}


void applyNewParams(
    std::map<Parameter, std::set<Entity>>& pParameters,
    std::map<Parameter, std::set<Entity>>& pNewParameters)
{
  for (auto& currNewParam : pNewParameters)
    pParameters[currNewParam.first] = std::move(currNewParam.second);
}



std::optional<Entity> plusIntOrStr(const std::optional<Entity>& pNb1,
                                   const std::optional<Entity>& pNb2)
{
  if (!pNb1 || !pNb2 || pNb1->type != pNb2->type)
    return {};
  try
  {
    auto nb1 = stringToNumber(pNb1->value);
    auto nb2 = stringToNumber(pNb2->value);
    return Entity(numberToString(nb1 + nb2), pNb1->type);
  } catch (...) {}
  return Entity(pNb1->value + pNb2->value, pNb1->type);
}


std::optional<Entity> minusIntOrStr(const std::optional<Entity>& pNb1,
                                    const std::optional<Entity>& pNb2)
{
  if (!pNb1 || !pNb2 || pNb1->type != pNb2->type)
    return {};
  try
  {
    auto nb1 = stringToNumber(pNb1->value);
    auto nb2 = stringToNumber(pNb2->value);
    return Entity(numberToString(nb1 - nb2), pNb1->type);
  } catch (...) {}
  return Entity(pNb1->value + "-" + pNb2->value, pNb1->type);
}


std::optional<Entity> multiplyNbOrStr(const std::optional<Entity>& pNb1,
                                      const std::optional<Entity>& pNb2)
{
  if (!pNb1 || !pNb2 || pNb1->type != pNb2->type)
    return {};
  try
  {
    auto nb1 = stringToNumber(pNb1->value);
    auto nb2 = stringToNumber(pNb2->value);
    return Entity(numberToString(nb1 * nb2), pNb1->type);
  } catch (...) {}
  return Entity(pNb1->value + "*" + pNb2->value, pNb1->type);
}


bool compIntNb(
    const std::string& pNb1Str,
    const Number& pNb2,
    bool pBoolSuperiorOrInferior,
    bool pCanBeEqual)
{
  try
  {
    auto nb1 = stringToNumber(pNb1Str);
    if (nb1 == pNb2)
      return pCanBeEqual;
    if (pBoolSuperiorOrInferior)
      return nb1 > pNb2;
    else
      return nb1 < pNb2;
  } catch (...) {}
  return false;
}

std::string incrementLastNumberUntilAConditionIsSatisfied(
    const std::string& pStr,
    const std::function<bool(const std::string&)>& pCondition)
{
  if (pCondition(pStr))
    return pStr;

  std::string base = pStr;
  std::size_t versionNb = 2;

  auto posUnderscore = pStr.find('_');
  if (posUnderscore != std::string::npos)
  {
    auto nbBeginOfPos = posUnderscore + 1;
    if (pStr.size() > nbBeginOfPos)
    {
      try
      {
        versionNb = _lexical_cast<std::size_t>(pStr.substr(nbBeginOfPos, pStr.size() - nbBeginOfPos));
        base = pStr.substr(0, posUnderscore);
      } catch (...) {}
    }
  }

  for (std::size_t i = 0; i < 1000000; ++i)
  {
    std::stringstream currentSs;
    currentSs << base << "_" << versionNb;
    auto currentStr = currentSs.str();
    if (pCondition(currentStr))
      return currentStr;
    ++versionNb;
  }
  throw std::runtime_error("incrementLastNumberUntilAConditionIsSatisfied(" + pStr + ") cannot find a condition statisfied");
}



void split(std::vector<std::string>& pStrs,
           const std::string& pStr,
           const std::string& pSeparator)
{
  std::string::size_type lastPos = 0u;
  std::string::size_type pos = lastPos;
  std::size_t separatorSize = pSeparator.size();
  while ((pos = pStr.find(pSeparator, pos)) != std::string::npos)
  {
    pStrs.emplace_back(pStr.substr(lastPos, pos - lastPos));
    pos += separatorSize;
    lastPos = pos;
  }
  pStrs.emplace_back(pStr.substr(lastPos, pStr.size() - lastPos));
}


std::list<Parameter> addParameter(const std::list<Parameter>* pParametersPtr,
                                  const std::optional<Parameter>& pParameterOpt)
{
  std::list<Parameter> parameters;
  if (pParametersPtr != nullptr)
    parameters.insert(parameters.end(), pParametersPtr->begin(), pParametersPtr->end());
  if (pParameterOpt)
    parameters.emplace_back(*pParameterOpt);
  return parameters;
}

// trim from start (in place)
void ltrim(std::string& s)
{
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
    return !std::isspace(ch);
  }));
}


// trim from end (in place)
void rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}


// trim from end (in place)
void trim(std::string& s)
{
    ltrim(s);
    rtrim(s);
}


std::set<Entity> typeToEntities(const Type& pParamType,
                                const SetOfEntities& pConstants,
                                const SetOfEntities& pObjects)
{
  std::set<Entity> res;
  _typeToEntitiesRec(res, pParamType, pConstants, pObjects);
  return res;
}


}
