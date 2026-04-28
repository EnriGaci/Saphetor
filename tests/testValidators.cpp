#include "VCFProcessor.h"

#include <gtest/gtest.h>
#include <regex>

using namespace Validations;

TEST(TestValidators, testHasValue) {
    HasValue validator(nullptr);
    EXPECT_ANY_THROW(validator.validate(""));
    EXPECT_NO_THROW(validator.validate("some value"));
}

TEST(TestValidators, testNoWhiteSpace) {
    NoWhiteSpace validator(nullptr);
    EXPECT_ANY_THROW(validator.validate("has space"));
    EXPECT_NO_THROW(validator.validate("nospaces"));
}

TEST(TestValidators, testChainDecorators) {
    // Chain: NoWhiteSpace -> HasValue
    auto validator = std::make_unique<NoWhiteSpace>(std::make_unique<HasValue>(nullptr));
    EXPECT_ANY_THROW(validator->validate("has space"));
    EXPECT_ANY_THROW(validator->validate("")); // HasValue triggers
    EXPECT_NO_THROW(validator->validate("nospaces"));
}

TEST(TestValidators, testNoSemicolon) {
    auto validator = std::make_unique<NoSemicolon>(nullptr);
    EXPECT_ANY_THROW(validator->validate("has;semicolon"));
    EXPECT_NO_THROW(validator->validate("nocolon"));
}

TEST(TestValidators, testMatchRegex) {
    std::regex rgx("^[ACGTN]+$", std::regex::icase);
    auto validator = std::make_unique<MatchRegex>(nullptr, rgx);
    EXPECT_NO_THROW(validator->validate("ACGT"));
    EXPECT_NO_THROW(validator->validate("nacgt")); // lower case, icase
    EXPECT_ANY_THROW(validator->validate("ACGTX")); // X not allowed
    EXPECT_ANY_THROW(validator->validate("AC GT")); // space not allowed
}

TEST(TestValidators, testMatchALTRegex) {
    std::regex rgx("^([ACGTNacgtn]+|\\*|\\.)$", std::regex::icase);
    auto validator = std::make_unique<MatchRegex>(nullptr, rgx);
    EXPECT_NO_THROW(validator->validate("ACGT"));
    EXPECT_NO_THROW(validator->validate("nacgt")); // lower case, icase
    EXPECT_NO_THROW(validator->validate(".")); // can be missing
    EXPECT_ANY_THROW(validator->validate("ACGTX")); // X not allowed
    EXPECT_ANY_THROW(validator->validate("AC GT")); // space not allowed
    EXPECT_ANY_THROW(validator->validate("AC,GT")); // comma not allowed
    EXPECT_ANY_THROW(validator->validate("AC>GT")); // > not allowed
    EXPECT_ANY_THROW(validator->validate("AC<GT")); // < not allowed
}

TEST(TestValidators, testRequired) {
    auto validator = std::make_unique<Required>(nullptr);
    EXPECT_NO_THROW(validator->validate("value"));
    EXPECT_ANY_THROW(validator->validate(".")); // . not allowed
}

TEST(TestValidators, testIsInteger) {
    auto validator = std::make_unique<IsInteger>(std::make_unique<HasValue>(nullptr));
    EXPECT_NO_THROW(validator->validate("123"));
    EXPECT_NO_THROW(validator->validate("0"));
    EXPECT_ANY_THROW(validator->validate("12.3")); // Not an integer
    EXPECT_ANY_THROW(validator->validate("abc"));
    EXPECT_ANY_THROW(validator->validate("")); // HasValue triggers
}

TEST(TestValidators, testIsFloat) {
    auto validator = std::make_unique<IsFloat>(std::make_unique<HasValue>(nullptr));
    EXPECT_NO_THROW(validator->validate("123.45"));
    EXPECT_NO_THROW(validator->validate("0.0"));
    EXPECT_NO_THROW(validator->validate("-1.23"));
    EXPECT_NO_THROW(validator->validate("1e3")); // scientific notation
    EXPECT_ANY_THROW(validator->validate("abc"));
    EXPECT_ANY_THROW(validator->validate("")); // HasValue triggers
    EXPECT_ANY_THROW(validator->validate("12.3.4")); // invalid float
}