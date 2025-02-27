#ifndef INCLUDE_ORDEREDGOALSPLANNER_SRC_WORLDSTATEMODIFICATIONPRIVATE_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_SRC_WORLDSTATEMODIFICATIONPRIVATE_HPP

#include <memory>
#include <optional>
#include <orderedgoalsplanner/types/worldstatemodification.hpp>
#include <orderedgoalsplanner/util/util.hpp>

namespace ogp
{

enum class WorldStateModificationNodeType
{
  AND,
  ASSIGN,
  FOR_ALL,
  INCREASE,
  DECREASE,
  MULTIPLY,
  PLUS,
  MINUS,
  WHEN
};


inline bool printAnyValueFor(WorldStateModificationNodeType pType) {
  return pType != WorldStateModificationNodeType::ASSIGN &&
        pType != WorldStateModificationNodeType::INCREASE && pType != WorldStateModificationNodeType::DECREASE &&
        pType != WorldStateModificationNodeType::MULTIPLY &&
        pType != WorldStateModificationNodeType::PLUS && pType != WorldStateModificationNodeType::MINUS &&
        pType != WorldStateModificationNodeType::WHEN;
}



enum class WsModificationPart
{
  AT_START,
  AT_END,
  POTENTIALLY_AT_END
};

struct WorldStateModificationNode : public WorldStateModification
{
  WorldStateModificationNode(WorldStateModificationNodeType pNodeType,
                             std::unique_ptr<WorldStateModification> pLeftOperand,
                             std::unique_ptr<WorldStateModification> pRightOperand,
                             const std::optional<Parameter>& pParameterOpt = {})
    : WorldStateModification(),
      nodeType(pNodeType),
      leftOperand(std::move(pLeftOperand)),
      rightOperand(std::move(pRightOperand)),
      parameterOpt(pParameterOpt),
      _successions()
  {
  }

  std::string toStr(bool pPrintAnyValue) const override;

  bool hasFact(const Fact& pFact) const override
  {
    return (leftOperand && leftOperand->hasFact(pFact)) ||
        (rightOperand && rightOperand->hasFact(pFact));
  }

  bool hasFactOptional(const ogp::FactOptional& FactOptional) const override
  {
    return (leftOperand && leftOperand->hasFactOptional(FactOptional)) ||
        (rightOperand && rightOperand->hasFactOptional(FactOptional));
  }

  void replaceArgument(const Entity& pOldFact,
                       const Entity& pNewFact) override
  {
    if (leftOperand)
      leftOperand->replaceArgument(pOldFact, pNewFact);
    if (rightOperand)
      rightOperand->replaceArgument(pOldFact, pNewFact);
  }

  void forAll(const std::function<void (const FactOptional&)>& pFactCallback,
              const SetOfFacts& pSetOfFact,
              const SetOfEntities& pConstants,
              const SetOfEntities& pObjects) const override;

  ContinueOrBreak forAllThatCanBeModified(const std::function<ContinueOrBreak (const FactOptional&)>& pFactCallback) const override;

  bool canSatisfyObjective(const std::function<bool (const FactOptional&, ParameterValuesWithConstraints*, const std::function<bool (const ParameterValuesWithConstraints&)>&)>& pFactCallback,
                           ParameterValuesWithConstraints& pParameters,
                           const WorldState& pWorldState,
                           const std::string& pFromDeductionId,
                           const SetOfEntities& pConstants,
                           const SetOfEntities& pObjects) const override;
  bool iterateOnSuccessions(const std::function<bool (const Successions&, const FactOptional&, ParameterValuesWithConstraints*, const std::function<bool (const ParameterValuesWithConstraints&)>&)>& pCallback,
                            ParameterValuesWithConstraints& pParameters,
                            const WorldState& pWorldState,
                            bool pCanSatisfyThisGoal,
                            const std::string& pFromDeductionId,
                            const SetOfEntities& pConstants,
                            const SetOfEntities& pObjects) const override;
  void updateSuccesions(const Domain& pDomain,
                        const WorldStateModificationContainerId& pContainerId,
                        const std::set<FactOptional>& pOptionalFactsToIgnore) override;
  void removePossibleSuccession(const ActionId& pActionIdToRemove) override;
  void getSuccesions(Successions& pSuccessions) const override;
  void printSuccesions(std::string& pRes) const override;

  bool operator==(const WorldStateModification& pOther) const override;

  std::optional<Entity> getValue(const SetOfFacts& pSetOfFact) const override;

  const FactOptional* getOptionalFact() const override
  {
    return nullptr;
  }

  std::unique_ptr<WorldStateModification> clone(const std::map<Parameter, Entity>* pParametersToArgumentPtr) const override
  {
    auto res = std::make_unique<WorldStateModificationNode>(
          nodeType,
          leftOperand ? leftOperand->clone(pParametersToArgumentPtr) : std::unique_ptr<WorldStateModification>(),
          rightOperand ? rightOperand->clone(pParametersToArgumentPtr) : std::unique_ptr<WorldStateModification>(),
          parameterOpt);
    res->_successions = _successions;
    return res;
  }

  bool hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                             std::list<Parameter>* pParametersPtr) const override;


  WorldStateModificationNodeType nodeType;
  std::unique_ptr<WorldStateModification> leftOperand;
  std::unique_ptr<WorldStateModification> rightOperand;
  std::optional<Parameter> parameterOpt;

private:
  Successions _successions;

  void _forAllInstruction(const std::function<void (const WorldStateModification&)>& pCallback,
                          const SetOfFacts& pSetOfFact,
                          ParameterValuesWithConstraints& pParameters,
                          const SetOfEntities& pConstants,
                          const SetOfEntities& pObjects) const;
};


struct WorldStateModificationFact : public WorldStateModification
{
  WorldStateModificationFact(const FactOptional& pFactOptional)
    : WorldStateModification(),
      factOptional(pFactOptional)
  {
  }

  std::string toStr(bool pPrintAnyValue) const override { return factOptional.toStr(nullptr, pPrintAnyValue); }

  bool hasFact(const ogp::Fact& pFact) const override
  {
    return factOptional.fact == pFact;
  }

  bool hasFactOptional(const ogp::FactOptional& FactOptional) const override
  {
    return factOptional == FactOptional;
  }

  void replaceArgument(const Entity& pOld,
                       const Entity& pNew) override
  {
    factOptional.fact.replaceArgument(pOld, pNew);
  }

  void forAll(const std::function<void (const FactOptional&)>& pFactCallback,
              const SetOfFacts&,
              const SetOfEntities&,
              const SetOfEntities&) const override { pFactCallback(factOptional); }

  ContinueOrBreak forAllThatCanBeModified(const std::function<ContinueOrBreak (const FactOptional&)>& pFactCallback) const override { return pFactCallback(factOptional); }

  bool canSatisfyObjective(const std::function<bool (const FactOptional&, ParameterValuesWithConstraints*, const std::function<bool (const ParameterValuesWithConstraints&)>&)>& pFactCallback,
                           ParameterValuesWithConstraints&,
                           const WorldState&,
                           const std::string&,
                           const SetOfEntities&,
                           const SetOfEntities&) const override
  {
    return pFactCallback(factOptional, nullptr, [](const ParameterValuesWithConstraints&){ return true; });
  }

  bool iterateOnSuccessions(const std::function<bool (const Successions&, const FactOptional&, ParameterValuesWithConstraints*, const std::function<bool (const ParameterValuesWithConstraints&)>&)>& pCallback,
                            ParameterValuesWithConstraints&,
                            const WorldState&,
                            bool pCanSatisfyThisGoal,
                            const std::string&,
                            const SetOfEntities&,
                            const SetOfEntities&) const override
  {
    if (pCanSatisfyThisGoal || !_successions.empty())
       return pCallback(_successions, factOptional, nullptr, [](const ParameterValuesWithConstraints&){ return true; });
    return false;
  }

  void updateSuccesions(const Domain& pDomain,
                        const WorldStateModificationContainerId& pContainerId,
                        const std::set<FactOptional>& pOptionalFactsToIgnore) override
  {
    _successions.clear();
    _successions.addSuccesionsOptFact(factOptional, pDomain, pContainerId, pOptionalFactsToIgnore);
  }

  void removePossibleSuccession(const ActionId& pActionIdToRemove) override
  {
    _successions.actions.erase(pActionIdToRemove);
  }

  void getSuccesions(Successions& pSuccessions) const override
  {
    pSuccessions.add(_successions);
  }

  void printSuccesions(std::string& pRes) const override
  {
    _successions.print(pRes, factOptional);
  }

  bool operator==(const WorldStateModification& pOther) const override;

  std::optional<Entity> getValue(const SetOfFacts& pSetOfFact) const override;

  const FactOptional* getOptionalFact() const override
  {
    return &factOptional;
  }

  std::unique_ptr<WorldStateModification> clone(const std::map<Parameter, Entity>* pParametersToArgumentPtr) const override
  {
    auto res = std::make_unique<WorldStateModificationFact>(factOptional);
    if (pParametersToArgumentPtr != nullptr)
      res->factOptional.fact.replaceArguments(*pParametersToArgumentPtr);
    res->_successions = _successions;
    return res;    return res;
  }

  bool hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                             std::list<Parameter>* pParametersPtr) const override;

  FactOptional factOptional;

private:
  Successions _successions;
};



struct WorldStateModificationNumber : public WorldStateModification
{
  WorldStateModificationNumber(const Number& pNb)
    : WorldStateModification(),
      _nb(pNb)
  {
  }

  static std::unique_ptr<WorldStateModificationNumber> create(const std::string& pStr);

  std::string toStr(bool) const override;

  bool hasFact(const ogp::Fact&) const override { return false; }
  bool hasFactOptional(const ogp::FactOptional&) const override { return false; }

  void replaceArgument(const Entity&,
                       const Entity&) override {}
  void forAll(const std::function<void (const FactOptional&)>&,
              const SetOfFacts&,
              const SetOfEntities&,
              const SetOfEntities&) const override {}
  ContinueOrBreak forAllThatCanBeModified(const std::function<ContinueOrBreak (const FactOptional&)>&) const override { return ContinueOrBreak::CONTINUE; }
  bool canSatisfyObjective(const std::function<bool (const FactOptional&, ParameterValuesWithConstraints*, const std::function<bool (const ParameterValuesWithConstraints&)>&)>&,
                           ParameterValuesWithConstraints&,
                           const WorldState&,
                           const std::string&,
                           const SetOfEntities&,
                           const SetOfEntities&) const override { return false; }
  bool iterateOnSuccessions(const std::function<bool (const Successions&, const FactOptional&, ParameterValuesWithConstraints*, const std::function<bool (const ParameterValuesWithConstraints&)>&)>&,
                            ParameterValuesWithConstraints&,
                            const WorldState&,
                            bool,
                            const std::string&,
                            const SetOfEntities&,
                            const SetOfEntities&) const override { return false; }
  void updateSuccesions(const Domain&,
                        const WorldStateModificationContainerId&,
                        const std::set<FactOptional>&) override {}
  void removePossibleSuccession(const ActionId&) override {}
  void getSuccesions(Successions&) const override {}
  void printSuccesions(std::string&) const override {}

  bool operator==(const WorldStateModification& pOther) const override;

  std::optional<Entity> getValue(const SetOfFacts&) const override
  {
    return Entity::createNumberEntity(toStr(true));
  }

  const FactOptional* getOptionalFact() const override
  {
    return nullptr;
  }

  std::unique_ptr<WorldStateModification> clone(const std::map<Parameter, Entity>*) const override
  {
    return std::make_unique<WorldStateModificationNumber>(_nb);
  }

  bool hasAContradictionWith(const std::set<FactOptional>&,
                             std::list<Parameter>*) const override { return false; }

  const Number& getNb() const { return _nb; }

private:
  Number _nb;
};

const WorldStateModificationNode* toWmNode(const WorldStateModification& pOther);

const WorldStateModificationFact* toWmFact(const WorldStateModification& pOther);

const WorldStateModificationNumber* toWmNumber(const WorldStateModification& pOther);


} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_SRC_WORLDSTATEMODIFICATIONPRIVATE_HPP
