#ifndef INCLUDE_ORDEREDGOALSPLANNER_ENTITY_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_ENTITY_HPP

#include <map>
#include <string>
#include <vector>
#include "../util/api.hpp"
#include "type.hpp"

namespace ogp
{
struct Ontology;
struct Parameter;
struct SetOfEntities;
struct SetOfTypes;


struct ORDEREDGOALSPLANNER_API Entity
{
  Entity(const std::string& pValue,
         const std::shared_ptr<Type>& pType);

  Entity(const Entity& pOther) = default;
  Entity(Entity&& pOther) noexcept;
  Entity& operator=(const Entity& pOther) = default;
  Entity& operator=(Entity&& pOther) noexcept;

  bool operator<(const Entity& pOther) const;

  bool operator==(const Entity& pOther) const;
  bool operator!=(const Entity& pOther) const { return !operator==(pOther); }

  static const std::string& anyEntityValue();
  static Entity createAnyEntity();
  static Entity createNumberEntity(const std::string& pNumber);
  static Entity fromDeclaration(const std::string& pStr,
                                const SetOfTypes& pSetOfTypes);

  static Entity fromUsage(const std::string& pStr,
                          const Ontology& pOntology,
                          const SetOfEntities& pObjects,
                          const std::vector<Parameter>& pParameters,
                          const std::map<std::string, Entity>* pParameterNamesToEntityPtr = nullptr);

  std::string toStr() const;
  bool isAnyEntity() const;
  bool isAParameterToFill() const;
  Parameter toParameter() const;
  bool match(const Parameter& pParameter) const;
  bool isValidParameterAccordingToPossiblities(const std::vector<Parameter>& pParameter) const;

  std::string value;
  std::shared_ptr<Type> type;
};

} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_ENTITY_HPP
