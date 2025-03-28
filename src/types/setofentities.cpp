#include <orderedgoalsplanner/types/setofentities.hpp>
#include <vector>
#include <orderedgoalsplanner/util/util.hpp>
#include <orderedgoalsplanner/types/goalstack.hpp>
#include <orderedgoalsplanner/types/setoftypes.hpp>
#include <orderedgoalsplanner/types/type.hpp>
#include <orderedgoalsplanner/types/worldstate.hpp>
namespace ogp
{

SetOfEntities::SetOfEntities()
    : _valueToEntity(),
      _typeNameToEntities()
{
}

SetOfEntities::Delta SetOfEntities::deltaFrom(const SetOfEntities& pOldSetOfEntities) const
{
  Delta res;
  for (const auto& currEntity : _valueToEntity)
  {
    auto it = pOldSetOfEntities._valueToEntity.find(currEntity.first);
    if (it == pOldSetOfEntities._valueToEntity.end())
      res.addedEntities.insert(currEntity.second);
  }

  for (const auto& currEntity : pOldSetOfEntities._valueToEntity)
  {
    auto it = _valueToEntity.find(currEntity.first);
    if (it == _valueToEntity.end())
      res.removedEntities.insert(currEntity.second);
  }
  return res;
}

SetOfEntities SetOfEntities::fromPddl(const std::string& pStr,
                                      const SetOfTypes& pSetOfTypes)
{
  SetOfEntities res;
  res.addAllFromPddl(pStr, pSetOfTypes);
  return res;
}

void SetOfEntities::add(const Entity& pEntity)
{
  _valueToEntity.erase(pEntity.value);
  _valueToEntity.emplace(pEntity.value, pEntity);

  if (pEntity.type)
    _typeNameToEntities[pEntity.type->name][pEntity];
}

void SetOfEntities::addAllIfNotExist(const SetOfEntities& pSetOfEntities)
{
  for (const auto& currValToEntity : pSetOfEntities._valueToEntity)
    if (_valueToEntity.count(currValToEntity.first) == 0)
       add(currValToEntity.second);
}

void SetOfEntities::addAllFromPddl(const std::string& pStr,
                                   const SetOfTypes& pSetOfTypes)
{
  std::vector<std::string> lineSplitted;
  ogp::split(lineSplitted, pStr, "\n");
  for (auto& currLine : lineSplitted)
  {
    std::vector<std::string> entitiesWithType;
    ogp::split(entitiesWithType, currLine, "-");

    if (entitiesWithType.empty())
      continue;

    if (entitiesWithType.size() <= 1) {
      trim(currLine);
      if (currLine.empty())
        continue;
      throw std::runtime_error("Missing type for entities declaration: \"" + currLine + "\"");
    }

    std::string typeStr = entitiesWithType[1];
    trim(typeStr);
    auto type = pSetOfTypes.nameToType(typeStr);
    if (!type)
      throw std::runtime_error("\"" + typeStr + "\" is not a valid type name in line \"" + currLine + "\"");

    auto entitiesStr = entitiesWithType[0];
    std::vector<std::string> entitiesStrs;
    ogp::split(entitiesStrs, entitiesStr, " ");
    for (auto& currEntity : entitiesStrs)
      if (!currEntity.empty())
        add(Entity(currEntity, type));
  }
}


void SetOfEntities::remove(const Entity& pEntity)
{
  _valueToEntity.erase(pEntity.value);

  if (pEntity.type)
  {
    auto& typeToEntities = _typeNameToEntities[pEntity.type->name];
   typeToEntities.erase(pEntity);
   if (typeToEntities.empty())
     _typeNameToEntities.erase(pEntity.type->name);
  }
}

const EntitiesWithParamConstaints* SetOfEntities::typeNameToEntities(const std::string& pTypename) const
{
  auto it = _typeNameToEntities.find(pTypename);
  if (it != _typeNameToEntities.end())
    return &it->second;
  return nullptr;
}

const Entity* SetOfEntities::valueToEntity(const std::string& pValue) const
{
  auto it = _valueToEntity.find(pValue);
  if (it != _valueToEntity.end())
    return &it->second;
  return nullptr;
}

std::string SetOfEntities::toStr(std::size_t pIdentation) const
{
  std::map<std::string, std::string> typeToValues;
  for (auto& currValueToEntity : _valueToEntity)
  {
    std::string typeName;
    if (currValueToEntity.second.type)
      typeName = currValueToEntity.second.type->name;
    auto& valuesStr = typeToValues[typeName];
    if (valuesStr != "")
      valuesStr += " ";
    valuesStr += currValueToEntity.second.value;
  }

  std::string res;
  bool firstIteration = true;
  for (auto& currTypeToValues : typeToValues)
  {
    if (firstIteration)
      firstIteration = false;
    else
      res += "\n";
    res += std::string(pIdentation, ' ') + currTypeToValues.second;
    if (currTypeToValues.first != "")
      res += " - " + currTypeToValues.first;
  }
  return res;
}


std::list<Entity> SetOfEntities::getUnusedEntitiesOfTypes(const WorldState& pWorldState,
                                                          const std::vector<Goal>& pGoals,
                                                          const std::vector<std::shared_ptr<Type>>& pTypes) const
{
  std::list<Entity> res;
  for (const auto& currType : pTypes)
  {
    auto itToEntities = _typeNameToEntities.find(currType->name);
    if (itToEntities != _typeNameToEntities.end())
    {
      auto& entitiesToConstraints = itToEntities->second;
      for (const auto& entityToConstraints : entitiesToConstraints)
      {
        const Entity& entity = entityToConstraints.first;
        if (!pWorldState.hasEntity(entity.value))
        {
          bool isInGoals = false;
          for (const Goal& currGoal : pGoals)
          {
            if (currGoal.objective().hasEntity(entity.value))
            {
              isInGoals = true;
              break;
            }
          }

          if (!isInGoals)
            res.emplace_back(entity);
        }
      }
    }
  }

  return res;
}



} // !ogp
