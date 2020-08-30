#include <ionshared/misc/util.h>
#include <ionlang/const/const.h>
#include <ionlang/const/const_name.h>
#include <ionlang/const/notice.h>
#include <ionlang/syntax/parser.h>
#include <ionlang/misc/util.h>

namespace ionlang {
    ionshared::OptPtr<Value<>> Parser::parseLiteral() {
        Token token = this->tokenStream.get();

        /**
         * Always use static pointer cast when downcasting to Value<>,
         * otherwise the cast result will be nullptr.
         */
        switch (token.getKind()) {
            case TokenKind::LiteralInteger: {
                ionshared::OptPtr<IntegerLiteral> integerValue = this->parseIntegerLiteral();

                if (ionshared::util::hasValue(integerValue)) {
                    return (*integerValue)->staticCast<Value<>>();
                }

                return std::nullopt;
            }

            case TokenKind::LiteralCharacter: {
                ionshared::OptPtr<CharLiteral> charValue = this->parseCharLiteral();

                if (ionshared::util::hasValue(charValue)) {
                    return (*charValue)->staticCast<Value<>>();
                }

                return std::nullopt;
            }

            // TODO: Missing values.

            default: {
                // TODO: Return std::nullopt instead.
                throw ionshared::util::quickError(IONLANG_NOTICE_MISC_UNEXPECTED_TOKEN);
            }
        }
    }

    ionshared::OptPtr<IntegerLiteral> Parser::parseIntegerLiteral() {
        IONIR_PARSER_EXPECT(TokenKind::LiteralInteger)

        /**
         * Abstract the token's value to be used in the
         * string to long integer conversion.
         */
        std::string tokenValue = this->tokenStream.get().getValue();

        // TODO: May stol() throw an error? If so, wrap in try-catch block for safety.
        /**
         * Attempt to convert token's value to a long
         * (int64_t for cross-platform support).
         */
        int64_t value;

        try {
            // TODO: Need to add support for 128-bit length.
            /**
             * May throw an exception if invalid arguments are provided,
             * or of the integer is too large to be held in any integer
             * type native to C++ (maximum is 64-bit length).
             */
            value = std::stoll(tokenValue);
        }
        catch (std::exception &exception) {
            // Value conversion failed.
            return this->makeNotice("Could not convert string to value, integer may be invalid or too large");
        }

        // Calculate the value's bit-length and it's corresponding integer kind.
        uint32_t valueBitLength = ionshared::util::calculateBitLength(value);

        std::optional<IntegerKind> valueIntegerKind =
            util::calculateIntegerKindFromBitLength(valueBitLength);

        if (!valueIntegerKind.has_value()) {
            return this->makeNotice("Integer value's type kind could not be determined");
        }

        // Create a long integer type for the value.
        ionshared::Ptr<IntegerType> type =
            std::make_shared<IntegerType>(*valueIntegerKind);

        // Create the integer instance.
        ionshared::Ptr<IntegerLiteral> integer =
            std::make_shared<IntegerLiteral>(type, value);

        // Skip current token.
        this->tokenStream.tryNext();

        // Finally, return the result.
        return integer;
    }

    ionshared::OptPtr<CharLiteral> Parser::parseCharLiteral() {
        IONIR_PARSER_EXPECT(TokenKind::LiteralCharacter);

        // Extract the value from the character token.
        std::string stringValue = this->tokenStream.get().getValue();

        // Skip over character token.
        this->tokenStream.skip();

        // Ensure extracted value only contains a single character.
        if (stringValue.length() > 1) {
            return this->makeNotice("Character value length must be at most 1 character");
        }

        // Create the character construct with the first and only character of the captured value.
        return std::make_shared<CharLiteral>(stringValue[0]);
    }

    ionshared::OptPtr<StringLiteral> Parser::parseStringLiteral() {
        IONIR_PARSER_EXPECT(TokenKind::LiteralString);

        // Extract the value from the string token.
        std::string value = this->tokenStream.get().getValue();

        // Skip over string token.
        this->tokenStream.skip();

        return std::make_shared<StringLiteral>(value);
    }
}
