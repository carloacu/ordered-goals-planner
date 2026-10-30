#include <orderedgoalsplanner/types/setoftypes.hpp>
#include <cctype>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <orderedgoalsplanner/util/util.hpp>


namespace ogp
{
namespace
{
const std::string _eitherPrefix = "(either";
const std::string _numberTypeName = "number";
const std::shared_ptr<Type> _numberType = std::make_shared<Type>(_numberTypeName);

void _removeAfterSemicolon(std::string& str) {
    size_t pos = str.find(';');
    if (pos != std::string::npos) {
        str.erase(pos);  // Erase from the semicolon to the end
    }
}

}

SetOfTypes::SetOfTypes()
    : _types(),
      _nameToType(),
      _nameToEitherType()
{
  _nameToType[_numberTypeName] = _numberType;
}


SetOfTypes SetOfTypes::fromPddl(const std::string& pStr)
{
  SetOfTypes res;
  res.addTypesFromPddl(pStr);
  return res;
}

void SetOfTypes::addType(const std::string& pTypeToAdd,
                         const std::string& pParentType)
{
  if (pParentType == "")
  {
    _types.push_back(std::make_shared<Type>(pTypeToAdd));
    _nameToType[pTypeToAdd] = _types.back();
    return;
  }

  auto it = _nameToType.find(pParentType);
  if (it == _nameToType.end())
  {
    addType(pParentType);
    it = _nameToType.find(pParentType);
  }

  auto type = std::make_shared<Type>(pTypeToAdd, it->second);
  it->second->subTypes.push_back(type);
  _nameToType[pTypeToAdd] = it->second->subTypes.back();
}


void SetOfTypes::addTypesFromPddl(const std::string& pStr)
{
  std::vector<std::string> lineSplitted;
  ogp::split(lineSplitted, pStr, "\n");

  for (auto& currLine : lineSplitted)
  {
    _removeAfterSemicolon(currLine);
    std::string typesStrs;
    std::string parentType;
    if (!splitPddlTypedList(typesStrs, parentType, currLine))
    {
      trim(typesStrs);
      if (typesStrs.empty())
        continue;
    }

    std::vector<std::string> types;
    ogp::split(types, typesStrs, " ");
    for (auto& currType : types)
      if (!currType.empty())
        addType(currType, parentType);
  }
}

std::shared_ptr<Type> SetOfTypes::nameToType(const std::string& pName) const
{
  auto it = _nameToType.find(pName);
  if (it != _nameToType.end())
    return it->second;
  if (pName.compare(0, _eitherPrefix.size(), _eitherPrefix) == 0)
    return _eitherType(pName);
  throw std::runtime_error("\"" + pName + "\" is not a valid type name");
}


std::shared_ptr<Type> SetOfTypes::_eitherType(const std::string& pName) const
{
  if (pName.size() <= _eitherPrefix.size() || pName.back() != ')' ||
      !std::isspace(static_cast<unsigned char>(pName[_eitherPrefix.size()])))
    throw std::runtime_error("\"" + pName + "\" is not a valid type name");

  std::istringstream memberNames(pName.substr(_eitherPrefix.size(), pName.size() - _eitherPrefix.size() - 1));
  std::map<std::string, std::shared_ptr<Type>> nameToMember;
  std::string currMemberName;
  while (memberNames >> currMemberName)
  {
    auto itMember = _nameToType.find(currMemberName);
    if (itMember == _nameToType.end())
      throw std::runtime_error("\"" + currMemberName + "\" is not a valid type name in \"" + pName + "\"");
    nameToMember.emplace(currMemberName, itMember->second);
  }
  if (nameToMember.empty())
    throw std::runtime_error("\"" + pName + "\" has no type to choose from");
  if (nameToMember.size() == 1)
    return nameToMember.begin()->second;

  std::string canonicalName = _eitherPrefix;
  std::list<std::shared_ptr<Type>> members;
  for (const auto& currNameToMember : nameToMember)
  {
    canonicalName += " " + currNameToMember.first;
    members.push_back(currNameToMember.second);
  }
  canonicalName += ")";

  auto itEither = _nameToEitherType.find(canonicalName);
  if (itEither != _nameToEitherType.end())
    return itEither->second;
  return _nameToEitherType.emplace(canonicalName, Type::createEither(canonicalName, members)).first->second;
}

std::shared_ptr<Type> SetOfTypes::numberType()
{
  return _numberType;
}


std::list<std::string> SetOfTypes::typesToStrs() const
{
  if (_types.empty())
    return {};
  std::list<std::string> res;
  for (auto& currType : _types)
    currType->toStrs(res);
  return res;
}


std::string SetOfTypes::toStr(std::size_t pIdentation) const
{
  std::string res;
  auto strs = typesToStrs();
  bool firstIteration = true;
  for (auto& currStr : strs)
  {
    if (firstIteration)
      firstIteration = false;
    else
      res += "\n";
    res += std::string(pIdentation, ' ') + currStr;
  }
  return res;
}

bool SetOfTypes::empty() const
{
  return _types.empty();
}


} // !ogp
