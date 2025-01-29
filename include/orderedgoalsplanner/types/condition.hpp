#ifndef INCLUDE_ORDEREDGOALSPLANNER_CONDITION_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_CONDITION_HPP

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include "../util/api.hpp"
#include "factoptional.hpp"
#include <orderedgoalsplanner/util/continueorbreak.hpp>
#include <orderedgoalsplanner/util/util.hpp>

namespace ogp
{
struct WorldState;
struct ConditionNode;
struct ConditionExists;
struct ConditionForall;
struct ConditionNot;
struct ConditionFact;
struct ConditionNumber;
struct SetOfDerivedPredicates;
struct Entity;
struct Parameter;
struct ExpressionParsed;


enum class ConditionPart
{
  AT_START,
  OVER_ALL
};

/// Condition with a tree structure to check in a world.
struct ORDEREDGOALSPLANNER_API Condition
{
  /// Because it is a parent struct.
  virtual ~Condition() = default;

  /**
   * @brief Convert the condition to a string.
   * @param[in] pFactWriterPtr Specific function to use to convert a fact to a string.
   * @return Condition converted to a string.
   */
  virtual std::string toStr(const std::function<std::string(const Fact&)>* pFactWriterPtr = nullptr,
                            bool pPrintAnyValue = true) const = 0;

  /// Check if this condition contains a fact or the negation of the fact.
  virtual bool hasFact(const Fact& pFact) const = 0;

  virtual bool hasEntity(const std::string& pEntityId) const = 0;

  /**
   * @brief Iterate over all the optional facts.
   * @param[in] pFactCallback Callback called for each optional fact of the condition.
   * @param[in] pIsWrappingExpressionNegated Is the expression wrapping this call is negated.
   */
  virtual ContinueOrBreak forAll(const std::function<ContinueOrBreak (const FactOptional&, bool)>& pFactCallback,
                                 bool pIsWrappingExpressionNegated = false,
                                 bool pIgnoreValue = false,
                                 bool pOnlyMandatoryFacts = false) const = 0;

  bool isOptFactMandatory(const FactOptional& pFactOptional,
                          bool pIgnoreValue = false) const;

  /**
   * @brief Find a condition fact candidate a fact that is in the effect of the preceding action.
   * @param[in] pDoesConditionFactMatchFactFromEffect Callback called for each optional fact until the callback returns true.
   * @param[in] pWorldState World state to consider.
   * @param[in] pFactFromEffect Fact that is in the effect of the preceding action.
   * @param[in] pFactFromEffectParameters pFactFromEffect parameters.
   * @param[in] pFactFromEffectTmpParametersPtr pFactFromEffect other parameters.
   * @param[in] pConditionParametersToPossibleArguments Map of the parameters of the condition to their possible arguments.
   * @param[in] pIsWrappingExpressionNegated Is the expression wrapping this call is negated.
   * @return True if one callback returned true, false otherwise.
   */
  virtual bool findConditionCandidateFromFactFromEffect(
      const std::function<bool (const FactOptional&, const Fact*, const Fact*)>& pDoesConditionFactMatchFactFromEffect,
      const WorldState& pWorldState,
      const SetOfEntities& pConstants,
      const SetOfEntities& pObjects,
      const Fact& pFactFromEffect,
      const std::map<Parameter, std::set<Entity>>& pFactFromEffectParameters,
      const std::map<Parameter, std::set<Entity>>* pFactFromEffectTmpParametersPtr,
      const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
      bool pIsWrappingExpressionNegated = false) const = 0;

  /**
   * @brief Iterate over all the optional facts with fact value resolution according to the world sate until the callback returns false.
   * @param[in] pFactCallback Callback called for each optional fact until the callback returns false.
   * @param[in] pWorldState World state to consider.
   * @return False if one callback returned false, true otherwise.
   */
  virtual bool untilFalse(const std::function<bool (const FactOptional&)>& pFactCallback,
                          const SetOfFacts& pSetOfFact) const = 0;

  /**
   * @brief Check if this condition is true for a specific world state.
   * @param[in] pWorldState World state to consider.
   * @param[in] pPunctualFacts Punctual facts raised right now.
   * @param[in] pRemovedFacts Facts present in the world state but that should be consider as not present.
   * @param[in, out] pConditionParametersToPossibleArguments Map of the parameters of the condition to their possible arguments <br/>
   * refreshed if it is necessary to have the condition true.
   * @param[in] pIsWrappingExpressionNegated Is the expression wrapping this call is negated.
   * @return True if this condition is satisfied.
   */
  virtual bool isTrue(const WorldState& pWorldState,
                      const SetOfEntities& pConstants,
                      const SetOfEntities& pObjects,
                      const std::set<Fact>& pPunctualFacts = {},
                      const std::set<Fact>& pRemovedFacts = {},
                      std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments = nullptr,
                      bool pIsWrappingExpressionNegated = false) const = 0;

  /// Equality operators.
  virtual bool operator==(const Condition& pOther) const = 0;
  virtual bool operator!=(const Condition& pOther) const { return !operator==(pOther); }

  /**
   * @brief Convert this condition to a value.
   * @param[in] pSetOfFact Facts use to extract value of the facts.
   * @return The condition converted to an Entity value.
   */
  virtual std::optional<Entity> getValue(const SetOfFacts& pSetOfFact) const = 0;

  /**
   * @brief Create a copy of this condition with arguments filling (or not if pConditionParametersToArgumentPtr is nullptr).
   * @param pConditionParametersToArgumentPtr Parameters to replace by their argument in the new condition to create.
   * @param pInvert True if the cloned condition should be inverted.
   * @return A copy of this condition with arguments filling.
   */
  virtual std::unique_ptr<Condition> clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr = nullptr,
                                           bool pInvert = false,
                                           const SetOfDerivedPredicates* pDerivedPredicatesPtr = nullptr) const = 0;

  virtual bool hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                                     bool pIsWrappingExpressionNegated = false,
                                     std::list<Parameter>* pParametersPtr = nullptr) const = 0;

  std::set<FactOptional> getAllOptFacts() const;

  /// Cast to ConditionNode* is possible.
  virtual const ConditionNode* fcNodePtr() const = 0;
  virtual ConditionNode* fcNodePtr() = 0;

  /// Cast to ConditionExists* is possible.
  virtual const ConditionExists* fcExistsPtr() const = 0;
  virtual ConditionExists* fcExistsPtr() = 0;

  /// Cast to ConditionForall* is possible.
  virtual const ConditionForall* fcForallPtr() const = 0;
  virtual ConditionForall* fcForallPtr() = 0;

  /// Cast to ConditionNot* is possible.
  virtual const ConditionNot* fcNotPtr() const = 0;
  virtual ConditionNot* fcNotPtr() = 0;

  /// Cast to ConditionFact* is possible.
  virtual const ConditionFact* fcFactPtr() const = 0;
  virtual ConditionFact* fcFactPtr() = 0;

  /// Cast to ConditionNumber* is possible.
  virtual const ConditionNumber* fcNbPtr() const = 0;
  virtual ConditionNumber* fcNbPtr() = 0;
};


enum class ConditionNodeType
{
  EQUALITY,
  SUPERIOR,
  SUPERIOR_OR_EQUAL,
  INFERIOR,
  INFERIOR_OR_EQUAL,
  AND,
  OR,
  IMPLY,
  PLUS,
  MINUS
};


static bool canBeSuperior(ConditionNodeType pConditionNodeType) {
  return pConditionNodeType == ConditionNodeType::SUPERIOR ||
      pConditionNodeType == ConditionNodeType::SUPERIOR_OR_EQUAL;
}

static bool canBeEqual(ConditionNodeType pConditionNodeType) {
  return pConditionNodeType == ConditionNodeType::EQUALITY ||
      pConditionNodeType == ConditionNodeType::SUPERIOR_OR_EQUAL ||
      pConditionNodeType == ConditionNodeType::INFERIOR_OR_EQUAL;
}

/// Condition tree node that holds children.
struct ORDEREDGOALSPLANNER_API ConditionNode : public Condition
{
  std::string toStr(const std::function<std::string(const Fact&)>* pFactWriterPtr,
                    bool pPrintAnyValue) const override;

  ConditionNode(ConditionNodeType pNodeType,
                std::unique_ptr<Condition> pLeftOperand,
                std::unique_ptr<Condition> pRightOperand);

  bool hasFact(const Fact& pFact) const override;
  bool hasEntity(const std::string& pEntityId) const override;
  ContinueOrBreak forAll(const std::function<ContinueOrBreak (const FactOptional&, bool)>& pFactCallback,
                         bool pIsWrappingExpressionNegated,
                         bool pIgnoreValue,
                         bool pOnlyMandatoryFacts) const override;
  bool findConditionCandidateFromFactFromEffect(const std::function<bool (const FactOptional&, const Fact*, const Fact*)>& pDoesConditionFactMatchFactFromEffect,
                                                const WorldState& pWorldState,
                                                const SetOfEntities& pConstants,
                                                const SetOfEntities& pObjects,
                                                const Fact& pFactFromEffect,
                                                const std::map<Parameter, std::set<Entity>>& pFactFromEffectParameters,
                                                const std::map<Parameter, std::set<Entity>>* pFactFromEffectTmpParametersPtr,
                                                const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
                                                bool pIsWrappingExpressionNegated) const override;
  bool untilFalse(const std::function<bool (const FactOptional&)>& pFactCallback,
                  const SetOfFacts& pSetOfFact) const override;
  bool isTrue(const WorldState& pWorldState,
              const SetOfEntities& pConstants,
              const SetOfEntities& pObjects,
              const std::set<Fact>& pPunctualFacts,
              const std::set<Fact>& pRemovedFacts,
              std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
              bool pIsWrappingExpressionNegated) const override;
  bool operator==(const Condition& pOther) const override;

  std::optional<Entity> getValue(const SetOfFacts& pSetOfFact) const override;

  std::unique_ptr<Condition> clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                   bool pInvert,
                                   const SetOfDerivedPredicates* pDerivedPredicatesPtr) const override;

  bool hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                             bool pIsWrappingExpressionNegated,
                             std::list<Parameter>* pParametersPtr) const override;

  const ConditionNode* fcNodePtr() const override { return this; }
  ConditionNode* fcNodePtr() override { return this; }
  const ConditionExists* fcExistsPtr() const override { return nullptr; }
  ConditionExists* fcExistsPtr() override { return nullptr; }
  const ConditionForall* fcForallPtr() const override { return nullptr; }
  ConditionForall* fcForallPtr() override { return nullptr; }
  const ConditionNot* fcNotPtr() const override { return nullptr; }
  ConditionNot* fcNotPtr() override { return nullptr; }
  const ConditionFact* fcFactPtr() const override { return nullptr; }
  ConditionFact* fcFactPtr() override { return nullptr; }
  const ConditionNumber* fcNbPtr() const override { return nullptr; }
  ConditionNumber* fcNbPtr() override { return nullptr; }

  ConditionNodeType nodeType;
  std::unique_ptr<Condition> leftOperand;
  std::unique_ptr<Condition> rightOperand;
};


/// Condition tree to manage exists operator.
struct ORDEREDGOALSPLANNER_API ConditionExists : public Condition
{
  ConditionExists(const Parameter& pParameter,
                  std::unique_ptr<Condition> pCondition);

  std::string toStr(const std::function<std::string(const Fact&)>* pFactWriterPtr,
                    bool pPrintAnyValue) const override;

  bool hasFact(const Fact& pFact) const override;
  bool hasEntity(const std::string& pEntityId) const override;
  ContinueOrBreak forAll(const std::function<ContinueOrBreak (const FactOptional&, bool)>& pFactCallback,
                         bool pIsWrappingExpressionNegated,
                         bool pIgnoreValue,
                         bool pOnlyMandatoryFacts) const override;

  bool findConditionCandidateFromFactFromEffect(
      const std::function<bool (const FactOptional&, const Fact*, const Fact*)>& pDoesConditionFactMatchFactFromEffect,
      const WorldState& pWorldState,
      const SetOfEntities& pConstants,
      const SetOfEntities& pObjects,
      const Fact& pFactFromEffect,
      const std::map<Parameter, std::set<Entity>>& pFactFromEffectParameters,
      const std::map<Parameter, std::set<Entity>>* pFactFromEffectTmpParametersPtr,
      const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
      bool pIsWrappingExpressionNegated) const override;

  bool untilFalse(const std::function<bool (const FactOptional&)>&,
                  const SetOfFacts&) const override { return true; } // TODO
  bool isTrue(const WorldState& pWorldState,
              const SetOfEntities& pConstants,
              const SetOfEntities& pObjects,
              const std::set<Fact>& pPunctualFacts,
              const std::set<Fact>& pRemovedFacts,
              std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
              bool pIsWrappingExpressionNegated) const override;
  bool operator==(const Condition& pOther) const override;

  std::optional<Entity> getValue(const SetOfFacts&) const override { return {}; }

  std::unique_ptr<Condition> clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                   bool pInvert,
                                   const SetOfDerivedPredicates* pDerivedPredicatesPtr) const override;
  bool hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                             bool pIsWrappingExpressionNegated,
                             std::list<Parameter>* pParametersPtr) const override;

  const ConditionNode* fcNodePtr() const override { return nullptr; }
  ConditionNode* fcNodePtr() override { return nullptr; }
  const ConditionExists* fcExistsPtr() const override { return this; }
  ConditionExists* fcExistsPtr() override { return this; }
  const ConditionForall* fcForallPtr() const override { return nullptr; }
  ConditionForall* fcForallPtr() override { return nullptr; }
  const ConditionNot* fcNotPtr() const override { return nullptr; }
  ConditionNot* fcNotPtr() override { return nullptr; }
  const ConditionFact* fcFactPtr() const override { return nullptr; }
  ConditionFact* fcFactPtr() override { return nullptr; }
  const ConditionNumber* fcNbPtr() const override { return nullptr; }
  ConditionNumber* fcNbPtr() override { return nullptr; }

  /// Variable to check the existance in the condition
  Parameter parameter;
  /// Expression to check
  std::unique_ptr<Condition> condition;
};


/// Condition tree to manage forall operator.
struct ORDEREDGOALSPLANNER_API ConditionForall : public Condition
{
  ConditionForall(const Parameter& pParameter,
                  std::unique_ptr<Condition> pCondition);

  std::string toStr(const std::function<std::string(const Fact&)>* pFactWriterPtr,
                    bool pPrintAnyValue) const override;

  bool hasFact(const Fact& pFact) const override;
  bool hasEntity(const std::string& pEntityId) const override;
  ContinueOrBreak forAll(const std::function<ContinueOrBreak (const FactOptional&, bool)>& pFactCallback,
                         bool pIsWrappingExpressionNegated,
                         bool pIgnoreValue,
                         bool pOnlyMandatoryFacts) const override;

  bool findConditionCandidateFromFactFromEffect(
      const std::function<bool (const FactOptional&, const Fact*, const Fact*)>& pDoesConditionFactMatchFactFromEffect,
      const WorldState& pWorldState,
      const SetOfEntities& pConstants,
      const SetOfEntities& pObjects,
      const Fact& pFactFromEffect,
      const std::map<Parameter, std::set<Entity>>& pFactFromEffectParameters,
      const std::map<Parameter, std::set<Entity>>* pFactFromEffectTmpParametersPtr,
      const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
      bool pIsWrappingExpressionNegated) const override;

  bool untilFalse(const std::function<bool (const FactOptional&)>&,
                  const SetOfFacts&) const override { return true; } // TODO
  bool isTrue(const WorldState& pWorldState,
              const SetOfEntities& pConstants,
              const SetOfEntities& pObjects,
              const std::set<Fact>& pPunctualFacts,
              const std::set<Fact>& pRemovedFacts,
              std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
              bool pIsWrappingExpressionNegated) const override;
  bool operator==(const Condition& pOther) const override;

  std::optional<Entity> getValue(const SetOfFacts&) const override { return {}; }

  std::unique_ptr<Condition> clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                   bool pInvert,
                                   const SetOfDerivedPredicates* pDerivedPredicatesPtr) const override;
  bool hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                             bool pIsWrappingExpressionNegated,
                             std::list<Parameter>* pParametersPtr) const override;

  const ConditionNode* fcNodePtr() const override { return nullptr; }
  ConditionNode* fcNodePtr() override { return nullptr; }
  const ConditionExists* fcExistsPtr() const override { return nullptr; }
  ConditionExists* fcExistsPtr() override { return nullptr; }
  const ConditionForall* fcForallPtr() const override { return this; }
  ConditionForall* fcForallPtr() override { return this; }
  const ConditionNot* fcNotPtr() const override { return nullptr; }
  ConditionNot* fcNotPtr() override { return nullptr; }
  const ConditionFact* fcFactPtr() const override { return nullptr; }
  ConditionFact* fcFactPtr() override { return nullptr; }
  const ConditionNumber* fcNbPtr() const override { return nullptr; }
  ConditionNumber* fcNbPtr() override { return nullptr; }

  /// Variable to check the forall in the condition
  Parameter parameter;
  /// Expression to check
  std::unique_ptr<Condition> condition;
};


/// Condition tree node that holds the negation of a condition.
struct ORDEREDGOALSPLANNER_API ConditionNot : public Condition
{
  ConditionNot(std::unique_ptr<Condition> pCondition);

  std::string toStr(const std::function<std::string(const Fact&)>* pFactWriterPtr,
                    bool pPrintAnyValue) const override;

  bool hasFact(const Fact& pFact) const override;
  bool hasEntity(const std::string& pEntityId) const override;
  ContinueOrBreak forAll(const std::function<ContinueOrBreak (const FactOptional&, bool)>& pFactCallback,
                         bool pIsWrappingExpressionNegated,
                         bool pIgnoreValue,
                         bool pOnlyMandatoryFacts) const override;

  bool findConditionCandidateFromFactFromEffect(
      const std::function<bool (const FactOptional&, const Fact*, const Fact*)>& pDoesConditionFactMatchFactFromEffect,
      const WorldState& pWorldState,
      const SetOfEntities& pConstants,
      const SetOfEntities& pObjects,
      const Fact& pFactFromEffect,
      const std::map<Parameter, std::set<Entity>>& pFactFromEffectParameters,
      const std::map<Parameter, std::set<Entity>>* pFactFromEffectTmpParametersPtr,
      const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
      bool pIsWrappingExpressionNegated) const override;

  bool untilFalse(const std::function<bool (const FactOptional&)>& pFactCallback,
                  const SetOfFacts& pSetOfFact) const override { return true; } // TODO
  bool isTrue(const WorldState& pWorldState,
              const SetOfEntities& pConstants,
              const SetOfEntities& pObjects,
              const std::set<Fact>& pPunctualFacts,
              const std::set<Fact>& pRemovedFacts,
              std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
              bool pIsWrappingExpressionNegated) const override;
  bool operator==(const Condition& pOther) const override;

  std::optional<Entity> getValue(const SetOfFacts&) const override { return {}; }

  std::unique_ptr<Condition> clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                   bool pInvert,
                                   const SetOfDerivedPredicates* pDerivedPredicatesPtr) const override;
  bool hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                             bool pIsWrappingExpressionNegated,
                             std::list<Parameter>* pParametersPtr) const override;

  const ConditionNode* fcNodePtr() const override { return nullptr; }
  ConditionNode* fcNodePtr() override { return nullptr; }
  const ConditionExists* fcExistsPtr() const override { return nullptr; }
  ConditionExists* fcExistsPtr() override { return nullptr; }
  const ConditionForall* fcForallPtr() const override { return nullptr; }
  ConditionForall* fcForallPtr() override { return nullptr; }
  const ConditionNot* fcNotPtr() const override { return this; }
  ConditionNot* fcNotPtr() override { return this; }
  const ConditionFact* fcFactPtr() const override { return nullptr; }
  ConditionFact* fcFactPtr() override { return nullptr; }
  const ConditionNumber* fcNbPtr() const override { return nullptr; }
  ConditionNumber* fcNbPtr() override { return nullptr; }

  /// Negated condition
  std::unique_ptr<Condition> condition;
};


/// Condition tree node that holds only an optional fact.
struct ORDEREDGOALSPLANNER_API ConditionFact : public Condition
{
  ConditionFact(const FactOptional& pFactOptional);

  std::string toStr(const std::function<std::string(const Fact&)>* pFactWriterPtr,
                    bool pPrintAnyValue) const override { return factOptional.toStr(pFactWriterPtr, pPrintAnyValue); }

  bool hasFact(const Fact& pFact) const override;
  bool hasEntity(const std::string& pEntityId) const override;
  ContinueOrBreak forAll(const std::function<ContinueOrBreak (const FactOptional&, bool)>& pFactCallback,
                         bool pIsWrappingExpressionNegated,
                         bool pIgnoreValue,
                         bool pOnlyMandatoryFacts) const override;
  bool findConditionCandidateFromFactFromEffect(
      const std::function<bool (const FactOptional&, const Fact*, const Fact*)>& pDoesConditionFactMatchFactFromEffect,
      const WorldState&,
      const SetOfEntities&,
      const SetOfEntities&,
      const Fact&,
      const std::map<Parameter, std::set<Entity>>&,
      const std::map<Parameter, std::set<Entity>>*,
      const std::map<Parameter, std::set<Entity>>&,
      bool pIsWrappingExpressionNegated) const override;
  bool untilFalse(const std::function<bool (const FactOptional&)>& pFactCallback,
                  const SetOfFacts&) const override { return pFactCallback(factOptional); }
  bool isTrue(const WorldState& pWorldState,
              const SetOfEntities& pConstants,
              const SetOfEntities& pObjects,
              const std::set<Fact>& pPunctualFacts,
              const std::set<Fact>& pRemovedFacts,
              std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
              bool pIsWrappingExpressionNegated) const override;
  bool operator==(const Condition& pOther) const override;

  std::optional<Entity> getValue(const SetOfFacts& pSetOfFact) const override;

  std::unique_ptr<Condition> clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                   bool pInvert,
                                   const SetOfDerivedPredicates* pDerivedPredicatesPtr) const override;
  bool hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                             bool pIsWrappingExpressionNegated,
                             std::list<Parameter>* pParametersPtr) const override;

  const ConditionNode* fcNodePtr() const override { return nullptr; }
  ConditionNode* fcNodePtr() override { return nullptr; }
  const ConditionExists* fcExistsPtr() const override { return nullptr; }
  ConditionExists* fcExistsPtr() override { return nullptr; }
  const ConditionForall* fcForallPtr() const override { return nullptr; }
  ConditionForall* fcForallPtr() override { return nullptr; }
  const ConditionNot* fcNotPtr() const override { return nullptr; }
  ConditionNot* fcNotPtr() override { return nullptr; }
  const ConditionFact* fcFactPtr() const override { return this; }
  ConditionFact* fcFactPtr() override { return this; }
  const ConditionNumber* fcNbPtr() const override { return nullptr; }
  ConditionNumber* fcNbPtr() override { return nullptr; }

  FactOptional factOptional;
};



/// Condition tree node that holds only a number.
struct ORDEREDGOALSPLANNER_API ConditionNumber : public Condition
{
  ConditionNumber(const Number& pNb);

  std::string toStr(const std::function<std::string(const Fact&)>* pFactWriterPtr,
                    bool pPrintAnyValue) const override;

  bool hasFact(const Fact&) const override { return false; }
  bool hasEntity(const std::string&) const override { return false; }
  ContinueOrBreak forAll(const std::function<ContinueOrBreak (const FactOptional&, bool)>&, bool, bool, bool) const override { return ContinueOrBreak::CONTINUE; }
  bool findConditionCandidateFromFactFromEffect(
      const std::function<bool (const FactOptional&, const Fact*, const Fact*)>&,
      const WorldState&,
      const SetOfEntities&,
      const SetOfEntities&,
      const Fact&,
      const std::map<Parameter, std::set<Entity>>&,
      const std::map<Parameter, std::set<Entity>>*,
      const std::map<Parameter, std::set<Entity>>&,
      bool) const override { return true; }
  bool untilFalse(const std::function<bool (const FactOptional&)>&,
                  const SetOfFacts&) const override { return true; }
  bool isTrue(const WorldState&,
              const SetOfEntities&,
              const SetOfEntities&,
              const std::set<Fact>&,
              const std::set<Fact>&,
              std::map<Parameter, std::set<Entity>>*,
              bool pIsWrappingExpressionNegated) const override { return !pIsWrappingExpressionNegated; }
  bool operator==(const Condition& pOther) const override;

  std::optional<Entity> getValue(const SetOfFacts& pSetOfFact) const override;

  std::unique_ptr<Condition> clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                   bool pInvert,
                                   const SetOfDerivedPredicates* pDerivedPredicatesPtr) const override;
  bool hasAContradictionWith(const std::set<FactOptional>&, bool, std::list<Parameter>*) const override { return false; }

  const ConditionNode* fcNodePtr() const override { return nullptr; }
  ConditionNode* fcNodePtr() override { return nullptr; }
  const ConditionExists* fcExistsPtr() const override { return nullptr; }
  ConditionExists* fcExistsPtr() override { return nullptr; }
  const ConditionForall* fcForallPtr() const override { return nullptr; }
  ConditionForall* fcForallPtr() override { return nullptr; }
  const ConditionNot* fcNotPtr() const override { return nullptr; }
  ConditionNot* fcNotPtr() override { return nullptr; }
  const ConditionFact* fcFactPtr() const override { return nullptr; }
  ConditionFact* fcFactPtr() override { return nullptr; }
  const ConditionNumber* fcNbPtr() const override { return this; }
  ConditionNumber* fcNbPtr() override { return this; }

  Number nb;
};



} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_CONDITION_HPP
