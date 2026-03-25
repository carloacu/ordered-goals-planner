#include <orderedgoalsplanner/types/parameter.hpp>
#include <vector>
#include <orderedgoalsplanner/types/entity.hpp>
#include <orderedgoalsplanner/types/setoftypes.hpp>
#include <orderedgoalsplanner/util/util.hpp>


namespace ogp
{


Parameter::Parameter(const std::string& pName,
                     const std::shared_ptr<Type>& pType)
  : name(pName),
    type(pType)
{
}

Parameter::Parameter(Parameter&& pOther) noexcept
  : name(std::move(pOther.name)),
    type(pOther.type) {
}


Parameter& Parameter::operator=(Parameter&& pOther) noexcept {
  name = std::move(pOther.name);
  type = pOther.type;
  return *this;
}


bool Parameter::operator<(const Parameter& pOther) const {
   // Because 2 parameters cannot have the same name
   return name < pOther.name;
}


bool Parameter::operator==(const Parameter& pOther) const {
  return name == pOther.name && type == pOther.type;
}


Parameter Parameter::fromStr(const std::string& pStr,
                             const SetOfTypes& pSetOfTypes)
{
  std::string nameStr;
  std::string typeStr;
  const bool hasType = splitPddlTypedList(nameStr, typeStr, pStr);

  if (nameStr.empty())
    throw std::runtime_error("\"" + pStr + "\" is not a valid entity");

  if (hasType)
    return Parameter(nameStr, pSetOfTypes.nameToType(typeStr));

  if (pSetOfTypes.empty())
    return Parameter(nameStr, {});
  throw std::runtime_error("\"" + pStr + "\" parameter should declare a type");
}

Parameter Parameter::fromType(const std::shared_ptr<Type>& pType)
{
  return Parameter("?" + pType->name, pType);
}


std::string Parameter::toStr() const
{
  if (!type)
    return name;
  return name + " - " + type->name;
}

Entity Parameter::toEntity() const
{
  return Entity(name, type);
}

bool Parameter::isAParameterToFill() const
{
  return !name.empty() && (name[0] == '?' || name == Entity::anyEntityValue());
}



} // !ogp
