#pragma once

#include <unordered_set>
#include <regex>

static const int TOKEN_CHROM = 0;
static const int TOKEN_POS = 1;
static const int TOKEN_ID = 2;
static const int TOKEN_REF = 3;
static const int TOKEN_ALT = 4;
static const int TOKEN_QUAL = 5;
static const int TOKEN_FILTER = 6;
static const int TOKEN_INFO = 7;
static const int TOKEN_FORMAT = 8;
static const int MIN_EXPECTED_TOKENS_IN_LINE = 9;

namespace Validations {

    /*
    * Base class for validations
    * Each validation can have a nested validation to allow chaining multiple validations together
    * 
    * Implement validate methods to perform custom validations
    */
    class Validator {
    public:
        Validator(std::unique_ptr<Validator> validator=nullptr) : mValidator(std::move(validator)) { }
        virtual void validate(const std::string& /*val*/) { return; };
        virtual ~Validator() = default;
    protected:
        std::unique_ptr<Validator> mValidator{ nullptr };
    };

    class HasValue : public Validator {
    public:
        HasValue(std::unique_ptr<Validator> validator) : Validator(std::move(validator)) { }
        void validate(const std::string& token) override {
            if (token.empty()) {
                throw std::runtime_error("Required value is empty");
            }
            if (mValidator) {
                mValidator->validate(token);
            }
        }
    };

    class NoCharAllowed : public Validator {
    public:
        NoCharAllowed(std::unique_ptr<Validator> validator, char c) : Validator(std::move(validator)) , charToValidate(c) { }
        void validate(const std::string& token) override {
            if (token.find(charToValidate) != std::string::npos) {
                throw std::runtime_error(std::string("Invalid char") + charToValidate + " found");
            }
            if (mValidator) {
                mValidator->validate(token);
            }
        }

        virtual ~NoCharAllowed() = default;
    private:
        char charToValidate;

    };

    class NoWhiteSpace : public NoCharAllowed {
    public:
        NoWhiteSpace(std::unique_ptr<Validator> validator) : NoCharAllowed(std::move(validator), ' ') { }
    };

    class NoSemicolon : public NoCharAllowed {
    public:
        NoSemicolon(std::unique_ptr<Validator> validator) : NoCharAllowed(std::move(validator), ';') { }
    };

    class NoZero : public NoCharAllowed {
    public:
        NoZero(std::unique_ptr<Validator> validator) : NoCharAllowed(std::move(validator), '0') { }
    };

    class MatchRegex : public Validator {
    public:
        MatchRegex(std::unique_ptr<Validator> validator, const std::regex& rgx ) : Validator(std::move(validator)) , mRgx(rgx) { }

        void validate(const std::string& token) override {
            std::smatch match;

            if (!std::regex_search(token, match, mRgx)) {
                throw std::runtime_error("Invalid token regex format");
            }
            if (mValidator) {
                mValidator->validate(token);
            }
        }
    private:
        std::regex mRgx;
    };


    class NoDuplicates : public Validator {
    public:
        NoDuplicates(std::unique_ptr<Validator> validator) : Validator(std::move(validator)) { }
        void validate(const std::string& token) override {
            if (seen.count(token)) {
                throw std::runtime_error("Duplicate value found: " + token);
            }
            seen.insert(token);
            if (mValidator) {
                mValidator->validate(token);
            }
        };
    private:
        std::unordered_set<std::string> seen;
    };


    class Required : public Validator {
    public:
        Required(std::unique_ptr<Validator> validator) : Validator(std::move(validator)) { }
        void validate(const std::string& token) override {
            if (token == ".") {
                throw std::runtime_error("Required value is missing");
            }
            if (mValidator) {
                mValidator->validate(token);
            }
        }
    };

    class IsInteger : public Validator {
    public:
        IsInteger(std::unique_ptr<Validator> validator) : Validator(std::move(validator)) { }
        void validate(const std::string& token) override {
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
    };

    class IsFloat : public Validator {
    public:
        IsFloat(std::unique_ptr<Validator> validator) : Validator(std::move(validator)) { }
        void validate(const std::string& token) override {
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
    };

    /*
    * MUST BE USED AS TOP DECORATOR when used
    */
    class AllowValue : public Validator {
        public:
            AllowValue(std::unique_ptr<Validator> validator, const std::string& val) : Validator(std::move(validator)) , mAllowedValue(val) { }

            void validate(const std::string& token) override {
                if (token == mAllowedValue) {
                    return; // Allow missing value, skip further validation
                }
                if (mValidator) {
                    mValidator->validate(token);
                }
            }
        private:
            std::string mAllowedValue;
    };

    class AllowMissing : public AllowValue {
    public:
        AllowMissing(std::unique_ptr<Validator> validator) : AllowValue(std::move(validator), ".") {}
    };

    class AllowPass : public AllowValue {
    public:
        AllowPass(std::unique_ptr<Validator> validator) : AllowValue(std::move(validator), "PASS") {}
    };

    inline std::vector<int> idsForValidation = { TOKEN_CHROM, TOKEN_POS, TOKEN_REF, TOKEN_ALT, TOKEN_QUAL, TOKEN_FILTER };

    using ValidationsType = std::unordered_map<int, std::shared_ptr<Validator>>;

    inline ValidationsType DefaultValidations = {
        {
            TOKEN_CHROM, 
            std::make_unique<Required>(
                std::make_unique<NoWhiteSpace>(
                    std::make_unique<HasValue>(nullptr)
                )
            )
        },
        {
            TOKEN_POS, 
            std::make_unique<Required>(
                std::make_unique<IsInteger>(nullptr)
            )
        },
        {
            TOKEN_ID,
            std::make_unique<AllowMissing>(
                std::make_unique<NoDuplicates>(
                    std::make_unique<NoWhiteSpace>(
                        std::make_unique<NoSemicolon>(nullptr)
                    )
                )
            )
        },
        {
            TOKEN_REF,
            std::make_unique<Required>(
                std::make_unique<MatchRegex>(nullptr,std::regex("^[ACGTN]+$", std::regex::icase)
                )
            )
        },
        {
            TOKEN_ALT,
            std::make_unique<AllowMissing>(
                std::make_unique<MatchRegex>(nullptr,std::regex("^([ACGTNacgtn]+|\\*|\\.)$", std::regex::icase)
                )
            )
        },
        {
            TOKEN_QUAL,
            std::make_unique<AllowMissing>(
                std::make_unique<IsFloat>(nullptr)
            )
        }
    };

} // end namespace

