#ifndef INCLUDE_ORDEREDGOALSPLANNER_PREDICATE_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_PREDICATE_HPP

#include <optional>
#include <string>
#include <vector>
#include "../util/api.hpp"
#include "parameter.hpp"


namespace ogp
{

struct ORDEREDGOALSPLANNER_API Predicate
{
  Predicate(const std::string& pStr,
            bool pStrPddlFormated,
            const SetOfTypes& pSetOfTypes,
            std::size_t pBeginPos = 0,
            std::size_t* pResPos = nullptr);
  Predicate(const std::string& pName,
            const std::vector<Parameter>& pParameters,
            const std::shared_ptr<Type>& pValue);

  std::string toPddl() const;
  std::string toStr() const;

  bool operator==(const Predicate& pOther) const;
  bool operator!=(const Predicate& pOther) const { return !operator==(pOther); }

  /**
   * Prefix to automatically copy facts and fluents pointing to other predicate and functions.
   * It is usefull for imply conditions. To be sure that the plan will not try to invalidate the condition.
   */
  static const std::string& getImmutablePrefix();

  bool isImmutable() const;
  Predicate createImmutableCopy() const;

  /// Name of the predicate.
  std::string name;
  /// Argument types of the predicate.
  std::vector<Parameter> parameters;
  /// Value type of the predicate.
  std::shared_ptr<Type> value;
};

} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_PREDICATE_HPP
