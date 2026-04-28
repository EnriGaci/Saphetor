#pragma once
#include "VCFConstants.h"

#include <unordered_set>
#include <unordered_map>
#include <regex>

namespace Validations {

    /*
    * Base class for validations following Decorator pattern
    * Each validation can have a nested validation to allow chaining multiple validations together
    * 
    * Implement validate method to perform custom validations
    */
    class Validator {
    public:
        Validator(std::unique_ptr<Validator> validator=nullptr);
        virtual void validate(const std::string& /*val*/);;
        virtual ~Validator() = default;
    protected:
        std::unique_ptr<Validator> mValidator{ nullptr };
    };

    /**
     * @brief Checks that token is not empty.
     * @throws std::runtime_error if the token is empty.
     */
    class HasValue : public Validator {
    public:
        HasValue(std::unique_ptr<Validator> validator);
        void validate(const std::string& token) override;
    };

    /**
     * @brief Checks that token does not contain a specific character.
     * @throws std::runtime_error if the token contains the forbidden character.
     */
    class NoCharAllowed : public Validator {
    public:
        NoCharAllowed(std::unique_ptr<Validator> validator, char c);
        void validate(const std::string& token) override;
        virtual ~NoCharAllowed() = default;
    private:
        char charToValidate;
    };

    /**
     * @brief Checks that token does not contain whitespace characters.
     * @throws std::runtime_error if the token contains whitespace.
     */
    class NoWhiteSpace : public NoCharAllowed {
    public:
        NoWhiteSpace(std::unique_ptr<Validator> validator);
    };

    /**
     * @brief Checks that token does not contain a semicolon (';').
     * @throws std::runtime_error if the token contains a semicolon.
     */
    class NoSemicolon : public NoCharAllowed {
    public:
        NoSemicolon(std::unique_ptr<Validator> validator);
    };

    /**
     * @brief Checks that token does not contain the character '0'.
     * @throws std::runtime_error if the token contains '0'.
     */
    class NoZero : public NoCharAllowed {
    public:
        NoZero(std::unique_ptr<Validator> validator);
    };

    /**
     * @brief Checks that token matches a given regular expression.
     * @throws std::runtime_error if the token does not match the regex.
     */
    class MatchRegex : public Validator {
    public:
        MatchRegex(std::unique_ptr<Validator> validator, const std::regex& rgx );
        void validate(const std::string& token) override;
    private:
        std::regex mRgx;
    };

    /**
     * @brief Checks that token has not been seen before (no duplicates).
     * @throws std::runtime_error if the token is a duplicate.
     */
    class NoDuplicates : public Validator {
    public:
        NoDuplicates(std::unique_ptr<Validator> validator);
        void validate(const std::string& token) override;;
    private:
        std::unordered_set<std::string> seen;
    };

    /**
     * @brief Checks that token is present and not the "." (Missing).
     * @throws std::runtime_error if the token is missing.
     */
    class Required : public Validator {
    public:
        Required(std::unique_ptr<Validator> validator);
        void validate(const std::string& token) override;
    };

    /**
     * @brief Checks that token is a valid integer.
     * @throws std::runtime_error if the token is not an integer.
     */
    class IsInteger : public Validator {
    public:
        IsInteger(std::unique_ptr<Validator> validator);
        void validate(const std::string& token) override;
    };

    /**
     * @brief Checks that token is a valid floating-point number.
     * @throws std::runtime_error if the token is not a float.
     */
    class IsFloat : public Validator {
    public:
        IsFloat(std::unique_ptr<Validator> validator);
        void validate(const std::string& token) override;
    };

    /*
    * MUST BE USED AS TOP DECORATOR when used
    */
    /**
     * @brief Allows a specific value for the token, bypassing other checks if matched.
     * @throws std::runtime_error if the token does not match the allowed value and fails other checks.
     */
    class AllowValue : public Validator {
        public:
            AllowValue(std::unique_ptr<Validator> validator, const std::string& val);
            void validate(const std::string& token) override;
        private:
            std::string mAllowedValue;
    };

    /**
     * @brief Allows the token to be missing (the special value "."), bypassing other checks if matched.
     * @throws std::runtime_error if the token does not match the allowed value and fails other checks.
     */
    class AllowMissing : public AllowValue {
    public:
        AllowMissing(std::unique_ptr<Validator> validator);
    };

    /**
     * @brief Allows the token to be 'PASS', bypassing other checks if matched.
     * @throws std::runtime_error if the token does not match the allowed value and fails other checks.
     */
    class AllowPass : public AllowValue {
    public:
        AllowPass(std::unique_ptr<Validator> validator);
    };

    inline std::vector<int> idsForValidation = { TOKEN_CHROM, TOKEN_POS, TOKEN_REF, TOKEN_ALT, TOKEN_QUAL, TOKEN_FILTER };

    using ValidationsType = std::unordered_map<int, std::shared_ptr<Validator>>;

    inline const ValidationsType DefaultValidations = {
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

