#ifndef INCLUDE_ORDEREDGOALSPLANNER_FACTARGUMENT_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_FACTARGUMENT_HPP

#include <memory>
#include <optional>
#include <string>
#include "../util/api.hpp"
#include "entity.hpp"
#include "entitieswithparamconstraints.hpp"

namespace ogp
{
struct Fact;
struct Parameter;
struct SetOfFacts;


struct ORDEREDGOALSPLANNER_API FactArgument
{
  FactArgument(const Entity& pEntity);
  FactArgument(Entity&& pEntity);
  FactArgument(const Fact& pFact);
  FactArgument(Fact&& pFact);
  FactArgument(const FactArgument& pOther);
  FactArgument(FactArgument&& pOther) noexcept;
  ~FactArgument();

  FactArgument& operator=(const FactArgument& pOther);
  FactArgument& operator=(FactArgument&& pOther) noexcept;

  bool operator<(const FactArgument& pOther) const;
  bool operator==(const FactArgument& pOther) const;
  bool operator!=(const FactArgument& pOther) const { return !operator==(pOther); }

  bool isEntity() const { return _entity.has_value(); }
  bool isFluent() const { return _fluentArg != nullptr; }
  bool isAnyEntity() const { return _entity && _entity->isAnyEntity(); }
  bool isAParameterToFill() const { return _entity && _entity->isAParameterToFill(); }

  bool hasParameter() const;
  bool hasEntity(const std::string& pEntityId) const;
  bool match(const Parameter& pParameter) const;
  bool isValidParameterAccordingToPossiblities(const std::vector<Parameter>& pParameters) const;

  const Entity& entity() const;
  Entity& entity();
  const Fact* fluent() const { return _fluentArg.get(); }
  Fact* fluent() { return _fluentArg.get(); }
  std::shared_ptr<Type> type() const;
  Parameter toParameter() const;

  void replaceArguments(const std::map<Parameter, Entity>& pCurrentArgumentsToNewArgument);
  void replaceArguments(const ParameterValuesWithConstraints& pCurrentArgumentsToNewArgument);

  std::optional<Entity> tryToResolveToEntity(const SetOfFacts& pSetOfFacts) const;

  std::string valueStr() const;
  std::string toStr(bool pPrintAnyValue = true) const;
  std::string toPddl(bool pPrintAnyValue = true) const;

private:
  std::optional<Entity> _entity;
  std::unique_ptr<Fact> _fluentArg;
};

} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_FACTARGUMENT_HPP
