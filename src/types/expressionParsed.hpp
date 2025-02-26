#ifndef INCLUDE_ORDEREDGOALSPLANNER_EXPRESSION_PARSED_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_EXPRESSION_PARSED_HPP

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ogp
{
struct Entity;
struct FactOptional;
struct Ontology;
struct Parameter;
struct SetOfEntities;


struct ExpressionParsed
{
  std::string name;
  std::list<ExpressionParsed> arguments;
  std::string value;
  bool isValueNegated = false;
  bool isAFunction = false;
  std::set<std::string> tags;

  char separatorToFollowingExp = ' ';
  std::unique_ptr<ExpressionParsed> followingExpression;

  bool empty() const { return name.empty(); }

  ExpressionParsed clone() const;

  std::string toStr() const;

  FactOptional toFact(const Ontology& pOntology,
                      const SetOfEntities& pObjects,
                      const std::vector<Parameter>& pParameters,
                      bool pIsOkIfValueIsMissing,
                      const std::map<std::string, Entity>* pParameterNamesToEntityPtr) const;

  void extractMissingObjects(std::list<Entity>& pRes,
                             const Ontology& pOntology,
                             const SetOfEntities& pObjects) const;

  void extractParameters(std::list<Parameter>& pRes,
                         const Ontology& pOntology) const;

  static ExpressionParsed fromStr(const std::string& pStr,
                                  std::size_t& pPos);

  static ExpressionParsed fromPddl(const std::string& pStr,
                                   std::size_t& pPos,
                                   bool pCanHaveFollowingExpression);

  static void skipSpaces(const std::string& pStr,
                         std::size_t& pPos);

  void skipSpacesWithTagExtraction(const std::string& pStr,
                                   std::size_t& pPos);

  static void moveUntilEndOfLine(const std::string& pStr,
                                 std::size_t& pPos);

  void moveUntilEndOfLineWithTagExtraction(const std::string& pStr,
                                           std::size_t& pPos);

  static void moveUntilClosingParenthesis(const std::string& pStr,
                                          std::size_t& pPos);

  static std::string parseToken(const std::string& pStr,
                                std::size_t& pPos);

  static std::string parseTokenThatCanBeEmpty(const std::string& pStr,
                                              std::size_t& pPos);

  static bool isEndOfTokenSeparator(char pChar);
};


} // !ogp


#endif // INCLUDE_ORDEREDGOALSPLANNER_EXPRESSION_PARSED_HPP
