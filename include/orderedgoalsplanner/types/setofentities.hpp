#ifndef INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFENTITIES_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFENTITIES_HPP

#include "../util/api.hpp"
#include <list>
#include <map>
#include <set>
#include <string>
#include "entitieswithparamconstraints.hpp"

namespace ogp
{
struct Goal;
struct SetOfTypes;
struct WorldState;


struct ORDEREDGOALSPLANNER_API SetOfEntities
{
  SetOfEntities();

  struct Delta
  {
    std::set<Entity> addedEntities;
    std::set<Entity> removedEntities;
  };

  Delta deltaFrom(const SetOfEntities& pOldSetOfEntities) const;

  static SetOfEntities fromPddl(const std::string& pStr,
                               const SetOfTypes& pSetOfTypes);

  void add(const Entity& pEntity);

  void addAllIfNotExist(const SetOfEntities& pSetOfEntities);

  void addAllFromPddl(const std::string& pStr,
                      const SetOfTypes& pSetOfTypes);

  void remove(const Entity& pEntity);

  const EntitiesWithParamConstaints* typeNameToEntities(const std::string& pTypename) const;

  const Entity* valueToEntity(const std::string& pValue) const;

  std::string toStr(std::size_t pIdentation = 0) const;

  bool empty() const { return _valueToEntity.empty(); }

  std::list<Entity> getUnusedEntitiesOfTypes(const WorldState& pWorldState,
                                             const std::vector<Goal>& pGoals,
                                             const std::vector<std::shared_ptr<Type>>& pTypes) const;

private:
  std::map<std::string, Entity> _valueToEntity;
  std::map<std::string, EntitiesWithParamConstaints> _typeNameToEntities;
};

} // namespace ogp

#endif // INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFENTITIES_HPP
