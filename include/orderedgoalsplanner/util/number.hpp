#ifndef INCLUDE_ORDEREDGOALSPLANNER_UTIL_NUMBER_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_UTIL_NUMBER_HPP

#include "api.hpp"
#include <optional>
#include <string>
#include <variant>

namespace ogp
{

// A variant type that can hold either int or float
using Number = std::variant<int, float>;

std::optional<Number> stringToNumberOpt(const std::string& str);

// Function to convert a string to either an int or a float and store it in a variant
Number stringToNumber(const std::string& str);

// Overloaded operator for addition of two Number objects
Number operator+(const Number& lhs, const Number& rhs);

// Overloaded operator for substraction of two Number objects
Number operator-(const Number& lhs, const Number& rhs);

// Overloaded operator for multiplication of two Number objects
Number operator*(const Number& lhs, const Number& rhs);

// Overloaded operator for equality comparison of two Number objects
bool operator==(const Number& lhs, const Number& rhs);

// Overloaded operators for relational comparison of two Number objects
bool operator<(const Number& lhs, const Number& rhs);
bool operator>(const Number& lhs, const Number& rhs);

// Overloaded operator for addition assignment of two Number objects
Number& operator+=(Number& lhs, const Number& rhs);

Number min(const Number& pNb1, const Number& pNb2);
Number max(const Number& pNb1, const Number& pNb2);

// Function to convert a Number to a std::string
std::string numberToString(const Number& num);

ORDEREDGOALSPLANNER_API
bool isNumber(const std::string& str);


}

#endif // INCLUDE_ORDEREDGOALSPLANNER_UTIL_NUMBER_HPP
