#pragma once

#include <variant>
#include <ionshared/misc/util.h>
#include <ionlang/syntax/ast_result.h>
#include <ionlang/construct/expression/unary_operation.h>
#include <ionlang/construct/type/integer_type.h>
#include <ionlang/construct/statement.h>
#include <ionlang/lexical/token_kind.h>

namespace ionlang::util {
    [[nodiscard]] std::string resolveIntegerKindName(IntegerKind kind);

    [[nodiscard]] TypeKind resolveTypeKind(const std::string &id);

    [[nodiscard]] std::optional<IntegerKind> calculateIntegerKindFromBitLength(uint32_t bitLength) noexcept;

    [[nodiscard]] std::optional<std::string> findConstructId(const ionshared::Ptr<Construct> &construct);

    [[nodiscard]] std::optional<std::string> findStatementId(const ionshared::Ptr<Statement> &statement) noexcept;

    [[nodiscard]] std::optional<Operator> findOperator(TokenKind tokenKind);

    template<typename ...Args>
    [[nodiscard]] std::runtime_error makeAstError(std::string format, Args &&...args) {
        std::optional<std::string> formattedString =
            ionshared::util::formatStringA(format, args...);

        if (!formattedString.has_value()) {
            throw std::runtime_error("Could not format string");
        }

        return std::runtime_error(*formattedString);
    }

    template<typename T = Construct>
    [[nodiscard]] bool hasValue(AstResult<T> result) {
        return std::holds_alternative<T>(result);
    }

    template<typename T>
    [[nodiscard]] bool hasValue(AstPtrResult<T> result) {
        return std::holds_alternative<ionshared::Ptr<T>>(result)
            && std::get<ionshared::Ptr<T>>(result) != nullptr;
    }

    template<typename T = Construct>
    [[nodiscard]] T getResultValue(AstResult<T> result) {
        if (!util::hasValue<T>(result)) {
            throw std::runtime_error("Result has no value");
        }

        return std::get<T>(result);
    }

    template<typename T = Construct>
    [[nodiscard]] ionshared::Ptr<T> getResultValue(AstPtrResult<T> result) {
        ionshared::Ptr<T> value = util::getResultValue<ionshared::Ptr<T>>(result);

        if (value == nullptr) {
            throw std::runtime_error("Pointer value of result is nullptr");
        }

        return value;
    }

    template<typename TFrom, typename TTo = Construct>
    [[nodiscard]] AstPtrResult<TTo> castAstPtrResult(AstPtrResult<TFrom> fromResult, bool useDynamicPointerCast = true) {
        if (util::hasValue(fromResult)) {
            ionshared::Ptr<TFrom> fromValue = util::getResultValue<TFrom>(fromResult);
            ionshared::Ptr<TTo> toValue;

            /**
             * Certain cases require use of static pointer cast instead of dynamic,
             * otherwise it returns nullptr. An example of this would be a downcast
             * from 'Value<IntegerType>' to 'Value<>'. In that case, it's due to it's
             * template argument and the compiler not knowing how to convert it's
             * corresponding field 'type'.
             */
            if (useDynamicPointerCast) {
                toValue = std::dynamic_pointer_cast<TTo>(fromValue);
            }
            else {
                toValue = std::static_pointer_cast<TTo>(fromValue);
            }

            // Conversion failed; construct is nullptr.
            if (toValue == nullptr) {
                throw std::runtime_error("Pointer cast failed; resulting value is nullptr");
            }

            return toValue;
        }

        return std::get<ionshared::Ptr<ErrorMarker>>(fromResult);
    }
}
