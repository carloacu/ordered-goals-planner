#include <orderedgoalsplanner/types/entity.hpp>
#include <vector>
#include <orderedgoalsplanner/types/ontology.hpp>
#include <orderedgoalsplanner/types/parameter.hpp>
#include <orderedgoalsplanner/types/setofentities.hpp>
#include <orderedgoalsplanner/types/setoftypes.hpp>
#include <orderedgoalsplanner/util/util.hpp>


namespace ogp
{
namespace
{
const std::string _anyEntity = "*";
}

Entity::Entity(const std::string& pValue,
               const std::shared_ptr<Type>& pType)
 : value(pValue),
   type(pType)
{
}

Entity::Entity(Entity&& pOther) noexcept
  : value(std::move(pOther.value)),
    type(pOther.type) {
}


Entity& Entity::operator=(Entity&& pOther) noexcept {
    value = std::move(pOther.value);
    type = pOther.type;
    return *this;
}


bool Entity::operator<(const Entity& pOther) const {
  if (value != pOther.value)
    return value < pOther.value;
  if (type && !pOther.type)
    return true;
  if (!type && pOther.type)
    return false;
  if (type && pOther.type)
    return *type < *pOther.type;
  return false;
}


bool Entity::operator==(const Entity& pOther) const {
  return value == pOther.value && type == pOther.type;
}


const std::string& Entity::anyEntityValue()
{
  return _anyEntity;
}


Entity Entity::createAnyEntity()
{
  return Entity(_anyEntity, {});
}


Entity Entity::createNumberEntity(const std::string& pNumber)
{
  return Entity(pNumber, SetOfTypes::numberType());
}


Entity Entity::fromDeclaration(const std::string& pStr,
                               const SetOfTypes& pSetOfTypes)
{
  std::vector<std::string> nameWithType;
  ogp::split(nameWithType, pStr, "-");

  if (nameWithType.empty())
    throw std::runtime_error("\"" + pStr + "\" is not a valid entity");

  if (nameWithType.size() > 1)
  {
    auto valueStr = nameWithType[0];
    trim(valueStr);
    auto typeStr = nameWithType[1];
    trim(typeStr);
    return Entity(valueStr, pSetOfTypes.nameToType(typeStr));
  }

  if (pSetOfTypes.empty())
    return Entity(nameWithType[0], {});
  throw std::runtime_error("\"" + pStr + "\" entity should declare a type");
}


Entity Entity::fromUsage(const std::string& pStr,
                         const Ontology& pOntology,
                         const SetOfEntities& pEntities,
                         const std::vector<Parameter>& pParameters)
{
  if (pStr.empty())
    throw std::runtime_error("Empty entity usage");

  if (pStr[0] == '?')
  {
    for (const auto& currParam : pParameters)
      if (currParam.name == pStr)
        return Entity(currParam.name, currParam.type);
    throw std::runtime_error("The parameter \"" + pStr + "\" is unknown");
  }

  auto* entityPtr = pOntology.constants.valueToEntity(pStr);
  if (entityPtr != nullptr)
    return Entity(*entityPtr);

  entityPtr = pEntities.valueToEntity(pStr);
  if (entityPtr != nullptr)
    return Entity(*entityPtr);
  if (pStr == Entity::anyEntityValue())
    return Entity::createAnyEntity();
  if (isNumber(pStr))
    return Entity::createNumberEntity(pStr);
  throw std::runtime_error("\"" + pStr + "\" is not an entity value");
}


std::string Entity::toStr() const
{
  if (!type)
    return value;
  return value + " - " + type->name;
}

bool Entity::isAnyEntity() const
{
  return value == _anyEntity;
}

bool Entity::isAParameterToFill() const
{
  return !value.empty() && (value[0] == '?' || isAnyEntity());
}

Parameter Entity::toParameter() const
{
  return Parameter(value, type);
}


bool Entity::match(const Parameter& pParameter) const
{
  if (value == pParameter.name)
  {
    if (type && pParameter.type && !type->isA(*pParameter.type))
      return false;
    return true;
  }
  return false;
}


bool Entity::isValidParameterAccordingToPossiblities(const std::vector<Parameter>& pParameters) const
{
  for (auto& currParam : pParameters)
  {
    if (value == currParam.name)
    {
      if (type && currParam.type && !currParam.type->isA(*type))
        return false;
      return true;
    }
  }
  return false;
}



} // !ogp
