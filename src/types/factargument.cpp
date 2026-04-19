#include <orderedgoalsplanner/types/factargument.hpp>
#include <stdexcept>
#include <orderedgoalsplanner/types/fact.hpp>
#include <orderedgoalsplanner/types/setoffacts.hpp>

namespace ogp
{

FactArgument::FactArgument(const Entity& pEntity)
  : _entity(pEntity),
    _fluentArg()
{
}

FactArgument::FactArgument(Entity&& pEntity)
  : _entity(std::move(pEntity)),
    _fluentArg()
{
}

FactArgument::FactArgument(const Fact& pFact)
  : _entity(),
    _fluentArg(std::make_unique<Fact>(pFact))
{
}

FactArgument::FactArgument(Fact&& pFact)
  : _entity(),
    _fluentArg(std::make_unique<Fact>(std::move(pFact)))
{
}

FactArgument::FactArgument(const FactArgument& pOther)
  : _entity(pOther._entity),
    _fluentArg(pOther._fluentArg ? std::make_unique<Fact>(*pOther._fluentArg) : nullptr)
{
}

FactArgument::FactArgument(FactArgument&& pOther) noexcept
  : _entity(std::move(pOther._entity)),
    _fluentArg(std::move(pOther._fluentArg))
{
}

FactArgument::~FactArgument()
{
}

FactArgument& FactArgument::operator=(const FactArgument& pOther)
{
  _entity = pOther._entity;
  _fluentArg = pOther._fluentArg ? std::make_unique<Fact>(*pOther._fluentArg) : nullptr;
  return *this;
}

FactArgument& FactArgument::operator=(FactArgument&& pOther) noexcept
{
  _entity = std::move(pOther._entity);
  _fluentArg = std::move(pOther._fluentArg);
  return *this;
}

bool FactArgument::operator<(const FactArgument& pOther) const
{
  if (isEntity() != pOther.isEntity())
    return isEntity();
  if (_entity && pOther._entity)
    return *_entity < *pOther._entity;
  if (_fluentArg && pOther._fluentArg)
    return *_fluentArg < *pOther._fluentArg;
  return false;
}

bool FactArgument::operator==(const FactArgument& pOther) const
{
  if (isEntity() != pOther.isEntity())
    return false;
  if (_entity && pOther._entity)
    return *_entity == *pOther._entity;
  if (_fluentArg && pOther._fluentArg)
    return *_fluentArg == *pOther._fluentArg;
  return !_entity && !pOther._entity && !_fluentArg && !pOther._fluentArg;
}

bool FactArgument::hasParameter() const
{
  return (_entity && _entity->isAParameterToFill()) ||
      (_fluentArg && _fluentArg->hasAParameter());
}

bool FactArgument::hasEntity(const std::string& pEntityId) const
{
  if (_entity)
    return _entity->value == pEntityId && !_entity->isAParameterToFill();
  return _fluentArg && _fluentArg->hasEntity(pEntityId);
}

bool FactArgument::match(const Parameter& pParameter) const
{
  return _entity && _entity->match(pParameter);
}

bool FactArgument::isValidParameterAccordingToPossiblities(const std::vector<Parameter>& pParameters) const
{
  if (_entity)
    return _entity->isValidParameterAccordingToPossiblities(pParameters);
  return !_fluentArg || std::all_of(_fluentArg->arguments().begin(), _fluentArg->arguments().end(),
                                    [&](const FactArgument& pArgument) {
    return !pArgument.hasParameter() || pArgument.isValidParameterAccordingToPossiblities(pParameters);
  });
}

const Entity& FactArgument::entity() const
{
  if (!_entity)
    throw std::runtime_error("This fact argument is not an entity");
  return *_entity;
}

Entity& FactArgument::entity()
{
  if (!_entity)
    throw std::runtime_error("This fact argument is not an entity");
  return *_entity;
}

std::shared_ptr<Type> FactArgument::type() const
{
  if (_entity)
    return _entity->type;
  if (_fluentArg)
    return _fluentArg->predicate.value;
  return {};
}

Parameter FactArgument::toParameter() const
{
  if (!_entity)
    throw std::runtime_error("A fluent fact argument cannot be converted to a parameter");
  return _entity->toParameter();
}

void FactArgument::replaceArguments(const std::map<Parameter, Entity>& pCurrentArgumentsToNewArgument)
{
  if (_entity)
  {
    auto itValueParam = pCurrentArgumentsToNewArgument.find(_entity->toParameter());
    if (itValueParam != pCurrentArgumentsToNewArgument.end())
      _entity = itValueParam->second;
    return;
  }

  if (_fluentArg)
    _fluentArg->replaceArguments(pCurrentArgumentsToNewArgument);
}

void FactArgument::replaceArguments(const ParameterValuesWithConstraints& pCurrentArgumentsToNewArgument)
{
  if (_entity)
  {
    auto itValueParam = pCurrentArgumentsToNewArgument.find(_entity->toParameter());
    if (itValueParam != pCurrentArgumentsToNewArgument.end() && !itValueParam->second.empty())
      _entity = itValueParam->second.begin()->first;
    return;
  }

  if (_fluentArg)
    _fluentArg->replaceArguments(pCurrentArgumentsToNewArgument);
}

std::optional<Entity> FactArgument::tryToResolveToEntity(const SetOfFacts& pSetOfFacts) const
{
  if (_entity)
    return *_entity;
  if (_fluentArg)
    return pSetOfFacts.getFluentValue(*_fluentArg);
  return {};
}

std::string FactArgument::toStr(bool pPrintAnyValue) const
{
  if (_entity)
    return _entity->toStr();
  if (_fluentArg)
    return _fluentArg->toStr(pPrintAnyValue);
  return "";
}

std::string FactArgument::valueStr() const
{
  if (_entity)
    return _entity->value;
  if (_fluentArg)
    return _fluentArg->toStr();
  return "";
}

std::string FactArgument::toPddl(bool pPrintAnyValue) const
{
  if (_entity)
    return _entity->value;
  if (_fluentArg)
    return _fluentArg->toPddl(false, pPrintAnyValue);
  return "";
}

} // !ogp
