#ifndef INCLUDE_ORDEREDGOALSPLANNER_FACTOPTIONALSTOID_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_FACTOPTIONALSTOID_HPP

#include <functional>
#include <memory>
#include "../util/api.hpp"
#include <orderedgoalsplanner/util/continueorbreak.hpp>

namespace ogp
{
struct Condition;
struct Fact;
struct FactOptional;
struct FactsToId;

struct ORDEREDGOALSPLANNER_API FactOptionalsToId
{
public:
  FactOptionalsToId();
  FactOptionalsToId(const FactOptionalsToId& pOther);
  void operator=(const FactOptionalsToId& pOther);
  ~FactOptionalsToId();

  void add(const FactOptional& pFactOptional,
           const std::string& pId);
  bool add(const Condition& pCondition,
           const std::string& pId);

  ContinueOrBreak find(const std::function<ContinueOrBreak (const std::string&)>& pCallback,
                       const FactOptional& pFactOptional,
                       bool pIgnoreValue = false) const;

  ContinueOrBreak findFact(const std::function<ContinueOrBreak (const std::string&)>& pCallback,
                           const Fact& pFact,
                           bool pIsFactNegated = false,
                           bool pIgnoreValue = false,
                           bool pIncludeMatchingWithOlderValue = true) const;

private:
  /// Map of facts from condition to value.
  std::unique_ptr<FactsToId> _factsToValue;
  /// Map of negationed facts from condition to value.
  std::unique_ptr<FactsToId> _notFactsToValue;
};

} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_FACTOPTIONALSTOID_HPP
