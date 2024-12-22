#ifndef INCLUDE_ORDEREDGOALSPLANNER_FACTOPTIONAL_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_FACTOPTIONAL_HPP

#include <functional>
#include <string>
#include <vector>
#include <map>
#include "../util/api.hpp"
#include "fact.hpp"

namespace ogp
{

/// Fact that can be negated.
struct ORDEREDGOALSPLANNER_API FactOptional
{
  /**
   * @brief Construct an optional fact.
   * @param pFact Fact contained.
   * @param pIsFactNegated If the fact is negated.
   */
  FactOptional(const Fact& pFact,
               bool pIsFactNegated = false);

  FactOptional(bool pIsFactNegated,
               const std::string& pName,
               const std::vector<std::string>& pArgumentStrs,
               const std::string& pValueStr,
               bool pIsValueNegated,
               const Ontology& pOntology,
               const SetOfEntities& pEntities,
               const std::vector<Parameter>& pParameters,
               bool pIsOkIfValueIsMissing = false);

  /**
   * @brief Construct an optional fact from another one.
   * @param pOther Other optional fact.
   */
  FactOptional(const FactOptional& pOther,
               const std::map<Parameter, Entity>* pParametersPtr = nullptr);

  /**
   * @brief Construct an optional fact from a part of a string.
   * @param[in] pStr Input string to parse.
   * @param[in] pSeparatorPtr Character to indicate the end of the fact in the input string, nullptr can be set if there is only one.
   * @param[in] pBeginPos Begin position in the input string.
   * @param[out] pResPos Postion in the input string after the parse.
   */
  FactOptional(const std::string& pStr,
               const Ontology& pOntology,
               const SetOfEntities& pEntities,
               const std::vector<Parameter>& pParameters,
               std::size_t pBeginPos = 0,
               std::size_t* pResPos = nullptr);

  /// Specify an order beween optional facts. It alows to use this type as key of map containers.
  bool operator<(const FactOptional& pOther) const;

  /// Set content from another optional fact.
  void operator=(const FactOptional& pOther);

  /// Check equality with another optional fact.
  bool operator==(const FactOptional& pOther) const;
  /// Check not equality with another optional fact.
  bool operator!=(const FactOptional& pOther) const { return !operator==(pOther); }

  /// Serialize this optional fact to a string.
  std::string toStr(const std::function<std::string(const Fact&)>* pFactWriterPtr = nullptr,
                    bool pPrintAnyValue = true) const;

  std::string toPddl(bool pInEffectContext,
                     bool pPrintAnyValue = true) const;

  bool doesFactEffectOfSuccessorGiveAnInterestForSuccessor(const FactOptional& pOptFact) const;

  bool hasAContradictionWith(const std::set<FactOptional>& pFactsOpt,
                             std::list<Parameter>* pParametersPtr,
                             bool pIsWrappingExpressionNegated) const;

  /// Is the fact negated.
  bool isFactNegated;
  /// Fact contained in this goal.
  Fact fact;

private:
  void _simplify();
};

} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_FACTOPTIONAL_HPP
