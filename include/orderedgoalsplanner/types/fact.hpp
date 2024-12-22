#ifndef INCLUDE_ORDEREDGOALSPLANNER_FACT_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_FACT_HPP

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <set>
#include "../util/api.hpp"
#include <orderedgoalsplanner/types/entity.hpp>
#include <orderedgoalsplanner/types/predicate.hpp>

namespace ogp
{
struct FactOptional;
struct Ontology;
struct SetOfEntities;
struct SetOfFacts;

/// Knowledge that can be contained in a world.
struct ORDEREDGOALSPLANNER_API Fact
{
  /**
   * @brief Construct a fact.
   * @param[in] pStr Fact to construct serialized in a string.
   * @param[in] pSeparatorPtr Character to indicate the end of the fact in pStr. Nullptr can be set if there is only one fact in the string.
   * @param[out] pIsFactNegatedPtr Is the fact constructed negated or not.
   * @param[in] pBeginPos Begin position in pStr.
   * @param[out] pResPos End postion of the fact in pStr after the parsing.
   */
  Fact(const std::string& pStr,
       bool pStrPddlFormated,
       const Ontology& pOntology,
       const SetOfEntities& pEntities,
       const std::vector<Parameter>& pParameters,
       bool* pIsFactNegatedPtr = nullptr,
       std::size_t pBeginPos = 0,
       std::size_t* pResPos = nullptr,
       bool pIsOkIfValueIsMissing = false);

  Fact(const std::string& pName,
       const std::vector<std::string>& pArgumentStrs,
       const std::string& pValueStr,
       bool pIsValueNegated,
       const Ontology& pOntology,
       const SetOfEntities& pEntities,
       const std::vector<Parameter>& pParameters,
       bool pIsOkIfValueIsMissing = false);

  /// Destruct the fact.
  ~Fact();

  Fact(const Fact& pOther);
  Fact(Fact&& pOther) noexcept;
  Fact& operator=(const Fact& pOther);
  Fact& operator=(Fact&& pOther) noexcept;

  /// Specify an order beween facts. It alows to use this type as key of map containers.
  bool operator<(const Fact& pOther) const;

  /// Check equality with another fact.
  bool operator==(const Fact& pOther) const;
  /// Check not equality with another fact.
  bool operator!=(const Fact& pOther) const { return !operator==(pOther); }

  friend std::ostream& operator<<(std::ostream& os, const Fact& pFact);

  /**
   * @brief Check equality with another fact without considering the values.
   * @param[in] pFact Other fact to compare.
   * @param[in] pOtherFactParametersToConsiderAsAnyValuePtr Other fact arguments to consider as "any entity".
   * @param[in] pOtherFactParametersToConsiderAsAnyValuePtr2 Another set of other fact rguments to consider as "any entity".
   * @return True if the equality check succeeded.
   */
  bool areEqualWithoutValueConsideration(const Fact& pFact,
                                          const std::map<Parameter, std::set<Entity>>* pOtherFactParametersToConsiderAsAnyValuePtr = nullptr,
                                          const std::map<Parameter, std::set<Entity>>* pOtherFactParametersToConsiderAsAnyValuePtr2 = nullptr) const;

  /// Check equality with another fact without considering an argument.
  bool areEqualWithoutAnArgConsideration(const Fact& pFact,
                                         const std::string& pArgToIgnore) const;

  bool areEqualWithoutArgsAndValueConsideration(const Fact& pFact,
                                                const std::list<Parameter>* pParametersToIgnorePtr) const;


  /**
   * @brief Is equal to another Fact or if any of the 2 Facts have an "any entity" that can match.
   * @param[in] pOther Other fact to compare.
   * @param[in] pOtherFactArgumentsToConsiderAsAnyValuePtr Arguments to consider as "any entity".
   * @param[in] pOtherFactParametersToConsiderAsAnyValuePtr2 Another set of arguments to consider as "any entity".
   * @param[in] pThisArgumentsToConsiderAsAnyValuePtr Arguments of the this fact to consider as "any entity".
   * @return True if the 2 facts match, false otherwise.
   */
  bool areEqualExceptAnyValues(const Fact& pOther,
                               const std::map<Parameter, std::set<Entity>>* pOtherFactParametersToConsiderAsAnyValuePtr = nullptr,
                               const std::map<Parameter, std::set<Entity>>* pOtherFactParametersToConsiderAsAnyValuePtr2 = nullptr,
                               const std::vector<Parameter>* pThisFactParametersToConsiderAsAnyValuePtr = nullptr) const;

  /**
   * @brief Is equal to another Fact or if any of the 2 Facts have an "any entity" that can match and without looking at the value.
   * @param[in] pOther Other fact to compare.
   * @param[in] pOtherFactArgumentsToConsiderAsAnyValuePtr Arguments to consider as "any entity".
   * @param[in] pOtherFactParametersToConsiderAsAnyValuePtr2 Another set of arguments to consider as "any entity".
   * @return True if the 2 facts match, false otherwise.
   */
  bool areEqualExceptAnyValuesAndValue(const Fact& pOther,
                                        const std::map<Parameter, std::set<Entity>>* pOtherFactParametersToConsiderAsAnyValuePtr = nullptr,
                                        const std::map<Parameter, std::set<Entity>>* pOtherFactParametersToConsiderAsAnyValuePtr2 = nullptr) const;

  bool doesFactEffectOfSuccessorGiveAnInterestForSuccessor(const Fact& pFact) const;

  /**
   * @brief Is it a punctual fact.<br/>
   * A punctual fact is a fact that is considered punctually but never stored in the world.
   * @return True if the fact is punctual.
   */
  bool isPunctual() const;

  /**
   * @brief hasParameterOrValue Does this fact contains a specific string in his arguments or in his value.
   * @param pParameter[in] String that can match in the arguments or in the value of this fact.
   * @return True if the string matches in the arguments or in the value of this fact, false otherwise.
   */
  bool hasParameterOrValue(const Parameter& pParameter) const;

  bool hasAParameter(bool pIgnoreValue = false) const;

  /**
   * @brief Extract an argument from another instance of this fact.<br/>
   * Another instance of this fact means that the 2 facts have the same name, the same number of arguments and the same polarity (= negationed or not).
   * @param pParameter[in] Argument of this fact.
   * @param pExampleFact[in] Example Fact.
   * @return Argument of the other fact corresponding to the pParameter of this fact.
   */
  std::optional<Entity> tryToExtractArgumentFromExample(const Parameter& pParameter,
                                                        const Fact& pExampleFact) const;

  /**
   * @brief Extract an argument from another instance of this fact.<br/>
   * Another instance of this fact means that the 2 facts have the same name, the same number of arguments and the same polarity (= negationed or not).<br/>
   * This function ignores the values.
   * @param pParameter[in] Parameter of this fact.
   * @param pExampleFact[in] Example Fact.
   * @return Argument of the other fact corresponding to the Parameter of this fact.
   */
  std::optional<Entity> tryToExtractArgumentFromExampleWithoutValueConsideration(const Parameter& pParameter,
                                                                                 const Fact& pExampleFact) const;


  /**
   * @brief Replace some arguments by other ones.
   * @param pCurrentArgumentsToNewArgument[in] Map of current arguments to new argument to set.
   */
  void replaceArguments(const std::map<Parameter, Entity>& pCurrentArgumentsToNewArgument);

  /**
   * @brief Replace some arguments by other ones.
   * @param pCurrentArgumentsToNewArgument[in] Map of current arguments to new possible arguments to set.<br/>
   * Only the first new possible argument to set will be considered.
   */
  void replaceArguments(const std::map<Parameter, std::set<Entity>>& pCurrentArgumentsToNewArgument);

  std::string toPddl(bool pInEffectContext,
                     bool pPrintAnyValue = true) const;

  /// Serialize this fact to a string.
  std::string toStr(bool pPrintAnyValue = true) const;

  /**
   * @brief Construct a fact from a string.
   * @param pStr Input string.
   * @param pIsFactNegatedPtr Is the fact constructed negated or not.
   * @return Fact constructed.
   */
  static Fact fromStr(const std::string& pStr,
                      const Ontology& pOntology,
                      const SetOfEntities& pobjects,
                      const std::vector<Parameter>& pParameters,
                      bool* pIsFactNegatedPtr = nullptr);

  static Fact fromPddl(const std::string& pStr,
                       const Ontology& pOntology,
                       const SetOfEntities& pObjects,
                       const std::vector<Parameter>& pParameters,
                       std::size_t pBeginPos = 0,
                       std::size_t* pResPos = nullptr,
                       bool pIsOkIfValueIsMissing = false);

  /**
   * @brief Does the fact matches any of the other facts.
   * @param[in] pOtherFacts Set of other facts.
   * @param[in] pParametersAreForTheFact If true, get the parameters from the fact else get the parameters from the set of other facts.
   * @param[out] pNewParametersPtr New parameter possibilities corresponding of the found match.
   * @param[in] pParametersPtr Already known parameters.
   * @param[in, out] pParametersToModifyInPlacePtr Parameters to modify in place.
   * @param[in] pTriedToModifyParametersPtr True if pNewParametersPtr is nullptr and this function wanted to add new parameters.
   * @return True if the fact matches any of the other facts.
   */
  bool isInOtherFacts(const std::set<Fact>& pOtherFacts,
                      std::map<Parameter, std::set<Entity>>* pNewParametersPtr,
                      bool pCheckAllPossibilities,
                      const std::map<Parameter, std::set<Entity>>* pParametersPtr,
                      std::map<Parameter, std::set<Entity>>* pParametersToModifyInPlacePtr = nullptr,
                      bool* pTriedToModifyParametersPtr = nullptr) const;

  /**
   * @brief Does the fact matches any of the other facts.
   * @param[in] pOtherFacts Map of fact names to other facts.
   * @param[in] pParametersAreForTheFact If true, get the parameters from the fact else get the parameters from the set of other facts.
   * @param[out] pNewParametersPtr New parameter possibilities corresponding of the found match.
   * @param[in] pParametersPtr Already known parameters.
   * @param[in, out] pParametersToModifyInPlacePtr Parameters to modify in place.
   * @param[in] pTriedToModifyParametersPtr True if pNewParametersPtr is nullptr and this function wanted to add new parameters.
   * @return True if the fact matches any of the other facts.
   */
  bool isInOtherFactsMap(const SetOfFacts& pOtherFacts,
                         std::map<Parameter, std::set<Entity>>* pNewParametersPtr,
                         bool pCheckAllPossibilities,
                         const std::map<Parameter, std::set<Entity>>* pParametersPtr,
                         std::map<Parameter, std::set<Entity>>* pParametersToModifyInPlacePtr = nullptr,
                         bool* pTriedToModifyParametersPtr = nullptr) const;

  bool updateParameters(std::map<Parameter, std::set<Entity>>& pNewPotentialParameters,
                        std::map<Parameter, std::set<Entity>>& pNewPotentialParametersInPlace,
                        std::map<Parameter, std::set<Entity>>* pNewParametersPtr,
                        bool pCheckAllPossibilities,
                        const std::map<Parameter, std::set<Entity>>* pParametersPtr,
                        std::map<Parameter, std::set<Entity>>* pParametersToModifyInPlacePtr,
                        bool* pTriedToModifyParametersPtr) const;

  /**
   * @brief Does the fact matches the other fact.
   * @param[in] pOtherFact The other facts.
   * @param[out] pNewParameters New parameter possibilities corresponding of the found match.
   * @param[in] pParametersPtr Already known parameters.
   * @param[in, out] pParametersToModifyInPlacePtr Parameters to modify in place.
   * @return True if the fact matches the other fact.
   */
  bool isInOtherFact(const Fact& pOtherFact,
                     std::map<Parameter, std::set<Entity>>& pNewParameters,
                     const std::map<Parameter, std::set<Entity>>* pParametersPtr,
                     std::map<Parameter, std::set<Entity>>& pNewParametersInPlace,
                     const std::map<Parameter, std::set<Entity>>* pParametersToModifyInPlacePtr) const;

  /**
   * @brief Replace, in the arguments of this fact, a fact by another fact.
   * @param pCurrent Fact to search in the arguments.
   * @param pNew New fact to set in place of pCurrent.
   */
  void replaceArgument(const Entity& pCurrent,
                       const Entity& pNew);

  std::map<Parameter, Entity> extratParameterToArguments() const;

  const std::string& name() const { return _name; }
  const std::vector<Entity>& arguments() const { return _arguments; }
  const std::optional<Entity>& value() const { return _value; }
  bool isValueNegated() const { return _isValueNegated; }
  void setValueNegated(bool pIsValueNegated) { _isValueNegated = pIsValueNegated; }

  std::string factSignature() const;
  std::string generateFactSignature() const;
  void generateSignatureForAllUpperTypes(std::list<std::string>& pRes) const;
  void generateSignatureForSubAndUpperTypes(std::list<std::string>& pRes) const;

  void setArgumentType(std::size_t pIndex, const std::shared_ptr<Type>& pType);
  void setValueType(const std::shared_ptr<Type>& pType);
  void setValue(const std::optional<Entity>& pValue);
  void setValueFromStr(const std::string& pValueStr);

  bool isCompleteWithAnyEntityValue() const;


  Predicate predicate;

  /// Constant defining the "undefined" special value.
  static const Entity& getUndefinedEntity();
  /// Prefix to detect a punctual fact. (= fact that is considered punctually but not stored in the world)
  static const std::string& getPunctualPrefix();

private:
  /// Name of the fact.
  std::string _name;
  /// Arguments of the fact.
  std::vector<Entity> _arguments;
  /// Value of the fact.
  std::optional<Entity> _value;
  /// Is the value of the fact negated.
  bool _isValueNegated;
  std::string _factSignature;

  void _generateSignatureForAllSubTypes(std::list<std::string>& pRes) const;

  bool _updateParameters(std::map<Parameter, std::set<Entity>>* pNewParametersPtr,
                         std::map<Parameter, std::set<Entity>>& pNewPotentialParameters,
                         bool pCheckAllPossibilities,
                         const std::map<Parameter, std::set<Entity>>* pParametersPtr,
                         bool* pTriedToModifyParametersPtr) const;

  void _resetFactSignatureCache();

  void _finalizeInisilizationAndValidityChecks(bool pIsOkIfValueIsMissing);
};


} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_FACT_HPP
