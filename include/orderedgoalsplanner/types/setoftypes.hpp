#ifndef INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFTYPES_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFTYPES_HPP

#include "../util/api.hpp"
#include <list>
#include <map>
#include <memory>
#include <string>
#include "type.hpp"

namespace ogp
{

struct ORDEREDGOALSPLANNER_API SetOfTypes
{
  SetOfTypes();

  static SetOfTypes fromPddl(const std::string& pStr);

  void addType(const std::string& pTypeToAdd,
               const std::string& pParentType = "");
  void addTypesFromPddl(const std::string& pStr);

  /**
   * @brief Find a type by its name.
   * @param[in] pName Name of a declared type, or `(either a b)` for the union of declared types.
   * @return The type. A union is created on its first lookup, then the same one is returned, so
   * that the parameters and entities of that type compare equal.
   *
   * Not thread safe, although const: the first lookup of a union adds it to this set.
   */
  std::shared_ptr<Type> nameToType(const std::string& pName) const;
  static std::shared_ptr<Type> numberType();

  std::list<std::string> typesToStrs() const;
  std::string toStr(std::size_t pIdentation = 0) const;
  bool empty() const;

private:
  std::list<std::shared_ptr<Type>> _types;
  std::map<std::string, std::shared_ptr<Type>> _nameToType;
  /// Unions created by `nameToType`, by their name with the member names sorted.
  mutable std::map<std::string, std::shared_ptr<Type>> _nameToEitherType;

  std::shared_ptr<Type> _eitherType(const std::string& pName) const;
};

} // namespace ogp

#endif // INCLUDE_ORDEREDGOALSPLANNER_TYPES_SETOFTYPES_HPP
