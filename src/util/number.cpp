#include <orderedgoalsplanner/util/number.hpp>
#include <sstream>

namespace ogp
{

// Function to convert a string to either an int or a float and store it in a variant
std::optional<Number> stringToNumberOpt(const std::string& str) {
    std::istringstream iss(str);

    // Try to parse as an int first
    int intValue;
    if (iss >> intValue && iss.eof()) {
        return intValue;  // Successfully parsed as an int
    }

    // Clear error flags and try parsing as a float
    iss.clear();
    iss.str(str);

    float floatValue;
    if (iss >> floatValue && iss.eof()) {
        return floatValue;  // Successfully parsed as a float
    }

    // If neither works, throw an exception
    throw std::invalid_argument("Invalid number format: " + str);
}


Number stringToNumber(const std::string& str) {
    auto resOpt = stringToNumberOpt(str);
    if (resOpt)
      return *resOpt;
    throw std::invalid_argument("Invalid number format: " + str);
}

// Overloaded operator for addition of two Number objects
Number operator+(const Number& lhs, const Number& rhs) {
    return std::visit([](auto&& l, auto&& r) -> Number {
        return l + r;
    }, lhs, rhs);
}

// Overloaded operator for substraction of two Number objects
Number operator-(const Number& lhs, const Number& rhs) {
    return std::visit([](auto&& l, auto&& r) -> Number {
        return l - r;
    }, lhs, rhs);
}

// Overloaded operator for multiplication of two Number objects
Number operator*(const Number& lhs, const Number& rhs) {
    return std::visit([](auto&& l, auto&& r) -> Number {
        return l * r;
    }, lhs, rhs);
}

// Overloaded operator for equality comparison of two Number objects
bool operator==(const Number& lhs, const Number& rhs) {
    return std::visit([](auto&& l, auto&& r) -> bool {
        return static_cast<float>(l) == static_cast<float>(r);
    }, lhs, rhs);
}

// Overloaded operators for relational comparison of two Number objects
bool operator<(const Number& lhs, const Number& rhs) {
    return std::visit([](auto&& l, auto&& r) -> bool {
        return static_cast<float>(l) < static_cast<float>(r);
    }, lhs, rhs);
}

bool operator>(const Number& lhs, const Number& rhs) {
    return std::visit([](auto&& l, auto&& r) -> bool {
        return static_cast<float>(l) > static_cast<float>(r);
    }, lhs, rhs);
}

// Overloaded operator for addition assignment of two Number objects
Number& operator+=(Number& lhs, const Number& rhs) {
    lhs = std::visit([](auto&& l, auto&& r) -> Number {
        return l + r;
    }, lhs, rhs);
    return lhs;
}

Number min(const Number& pNb1, const Number& pNb2)
{
  if (pNb1 < pNb2)
    return pNb1;
  return pNb2;
}


Number max(const Number& pNb1, const Number& pNb2)
{
  if (pNb1 < pNb2)
    return pNb2;
  return pNb1;
}


// Function to convert a Number to a std::string
std::string numberToString(const Number& num) {
    return std::visit([](auto&& value) -> std::string {
        return std::to_string(value);
    }, num);
}


bool isNumber(const std::string& str) {
    if (str.empty()) return false; // Empty strings are not numbers

    size_t i = 0;
    if (str[i] == '-' || str[i] == '+') {
        ++i; // Skip the sign character
    }

    bool hasDigits = false;
    bool hasDecimalPoint = false;

    for (; i < str.size(); ++i) {
        if (std::isdigit(str[i])) {
            hasDigits = true; // Found at least one digit
        } else if (str[i] == '.') {
            if (hasDecimalPoint) {
                return false; // Multiple decimal points are invalid
            }
            hasDecimalPoint = true;
        } else {
            return false; // Invalid character
        }
    }

    return hasDigits; // Valid if there's at least one digit
}


}
