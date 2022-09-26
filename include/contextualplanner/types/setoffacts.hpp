#ifndef INCLUDE_CONTEXTUALPLANNER_TYPES_SETOFFACTS_HPP
#define INCLUDE_CONTEXTUALPLANNER_TYPES_SETOFFACTS_HPP

#include <list>
#include <set>
#include "../util/api.hpp"
#include <contextualplanner/types/fact.hpp>
#include <contextualplanner/types/expression.hpp>


namespace cp
{

/// Container of a set of fact modifications to apply in the world.
struct CONTEXTUALPLANNER_API SetOfFacts
{
  /// Construct an empty SetOfFacts.
  SetOfFacts()
    : facts(),
      notFacts(),
      exps()
  {
  }

  /**
   * @brief Construct a SetOfFacts
   * @param pFacts Facts to add when it will be applied in a world.
   * @param pNotFacts Facts to remove when it will be applied in a world.
   */
  SetOfFacts(const std::initializer_list<Fact>& pFacts,
             const std::initializer_list<Fact>& pNotFacts = {})
    : facts(pFacts),
      notFacts(pNotFacts),
      exps()
  {
  }


  /// Check if the set of facts is empty.
  bool empty() const { return facts.empty() && notFacts.empty() && exps.empty(); }

  /// Check equality with another SetOfFacts.
  bool operator==(const SetOfFacts& pOther) const
  { return facts == pOther.facts && notFacts == pOther.notFacts && exps == pOther.exps; }

  /**
   * @brief Add new values from another SetOfFacts object.
   * @param pOther Other SetOfFacts to add.
   */
  void add(const SetOfFacts& pOther);

  /**
   * @brief Check if SetOfFacts contains a fact even if it is a fact to remove when this object will be applied to a world.
   * @param pFact Fact to check.
   * @return True if this object contains the fact, false otherwise.
   */
  bool containsFact(const Fact& pFact) const;

  /**
   * @brief Check if this object is totally included in another SetOfFacts object.
   * @param pOther The other SetOfFacts to check.
   * @return True if this object is included, false otherwise.
   */
  bool isIncludedIn(const SetOfFacts& pOther) const;

  /**
   * @brief Change the name of a fact contained in this object.
   * @param pOldFact Current name of the fact.
   * @param pNewFact New name of the fact.
   */
  void rename(const Fact& pOldFact,
              const Fact& pNewFact);

  /// Convert this object to a list of key-value pairs serialized in strings.
  std::list<std::pair<std::string, std::string>> toFactsStrs() const;

  /// Convert this object to a list of strings.
  std::list<std::string> toStrs() const;

  /**
   * @brief Convert this object to string.
   * @param pSeparator Separation beween each facts.
   * @return String representing this object.
   */
  std::string toStr(const std::string& pSeparator) const;

  /**
   * @brief Convert a string to a SetOfFacts object.
   * @param pStr Input string to convert.
   * @param pSeparator Separator of facts in the input string.
   * @return The SetOfFacts object.
   */
  static SetOfFacts fromStr(const std::string& pStr,
                            char pSeparator);


  /// Facts to add when it will be applied in a world.
  std::set<Fact> facts;
  /// Facts to remove when it will be applied in a world.
  std::set<Fact> notFacts;
  /// Expressions to add when it will be applied in a world.
  std::list<Expression> exps;
};


} // !cp


#endif // INCLUDE_CONTEXTUALPLANNER_TYPES_SETOFFACTS_HPP