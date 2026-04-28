#include "Validators.h"

Validations::Validator::Validator(std::unique_ptr<Validator> validator) : mValidator(std::move(validator)) { }

void Validations::Validator::validate(const std::string&) { return; }

Validations::HasValue::HasValue(std::unique_ptr<Validator> validator) : Validator(std::move(validator)) { }

void Validations::HasValue::validate(const std::string& token) {
    if (token.empty()) {
        throw std::runtime_error("Required value is empty");
    }
    if (mValidator) {
        mValidator->validate(token);
    }
}

Validations::NoCharAllowed::NoCharAllowed(std::unique_ptr<Validator> validator, char c) : Validator(std::move(validator)), charToValidate(c) { }

void Validations::NoCharAllowed::validate(const std::string& token) {
    if (token.find(charToValidate) != std::string::npos) {
        throw std::runtime_error(std::string("Invalid char") + charToValidate + " found");
    }
    if (mValidator) {
        mValidator->validate(token);
    }
}

Validations::NoWhiteSpace::NoWhiteSpace(std::unique_ptr<Validator> validator) : NoCharAllowed(std::move(validator), ' ') { }

Validations::NoSemicolon::NoSemicolon(std::unique_ptr<Validator> validator) : NoCharAllowed(std::move(validator), ';') { }

Validations::NoZero::NoZero(std::unique_ptr<Validator> validator) : NoCharAllowed(std::move(validator), '0') { }

Validations::MatchRegex::MatchRegex(std::unique_ptr<Validator> validator, const std::regex& rgx) : Validator(std::move(validator)), mRgx(rgx) { }

void Validations::MatchRegex::validate(const std::string& token) {
    std::smatch match;

    if (!std::regex_search(token, match, mRgx)) {
        throw std::runtime_error("Invalid token regex format");
    }
    if (mValidator) {
        mValidator->validate(token);
    }
}

Validations::NoDuplicates::NoDuplicates(std::unique_ptr<Validator> validator) : Validator(std::move(validator)) { }

void Validations::NoDuplicates::validate(const std::string& token) {
    if (seen.count(token)) {
        throw std::runtime_error("Duplicate value found: " + token);
    }
    seen.insert(token);
    if (mValidator) {
        mValidator->validate(token);
    }
}

Validations::Required::Required(std::unique_ptr<Validator> validator) : Validator(std::move(validator)) { }

void Validations::Required::validate(const std::string& token) {
    if (token == ".") {
        throw std::runtime_error("Required value is missing");
    }
    if (mValidator) {
        mValidator->validate(token);
    }
}

Validations::IsInteger::IsInteger(std::unique_ptr<Validator> validator) : Validator(std::move(validator)) { }

void Validations::IsInteger::validate(const std::string& token) {
    try {
        size_t size;
        (void)std::stoi(token, &size);
        if (size != token.size()) {
            throw std::runtime_error("Value is not a valid integer");
        }
    }
    catch (const std::exception&) {
        throw std::runtime_error("Value is not a valid integer");
    }
    if (mValidator) {
        mValidator->validate(token);
    }
}

Validations::IsFloat::IsFloat(std::unique_ptr<Validator> validator) : Validator(std::move(validator)) { }

void Validations::IsFloat::validate(const std::string& token) {
    try {
        size_t size;
        (void)std::stof(token, &size);
        if (size != token.size()) {
            throw std::runtime_error("Value is not a valid float");
        }
    }
    catch (const std::exception&) {
        throw std::runtime_error("Value is not a valid float");
    }
    if (mValidator) {
        mValidator->validate(token);
    }
}

Validations::AllowValue::AllowValue(std::unique_ptr<Validator> validator, const std::string& val) : Validator(std::move(validator)), mAllowedValue(val) { }

void Validations::AllowValue::validate(const std::string& token) {
    if (token == mAllowedValue) {
        return; // Allow missing value, skip further validation
    }
    if (mValidator) {
        mValidator->validate(token);
    }
}

Validations::AllowMissing::AllowMissing(std::unique_ptr<Validator> validator) : AllowValue(std::move(validator), ".") {}

Validations::AllowPass::AllowPass(std::unique_ptr<Validator> validator) : AllowValue(std::move(validator), "PASS") {}
