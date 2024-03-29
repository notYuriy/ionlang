#include <ionlang/lexical/classifier.h>

namespace ionlang {
    bool Classifier::isSymbol(TokenKind tokenKind) {
        return TokenConst::getSymbols().contains(tokenKind);
    }

    bool Classifier::isNumeric(TokenKind tokenKind) {
        // TODO: Need to define numeric group.
        return false;
    }

    bool Classifier::isOperator(TokenKind tokenKind) {
        return TokenConst::getOperators().contains(tokenKind);
    }

    bool Classifier::isBuiltInType(TokenKind tokenKind) {
        return TokenConst::contains(TokenConst::getBuiltInTypes(), tokenKind);
    }

    bool Classifier::isUnsignedIntegerType(TokenKind tokenKind) {
//        return tokenKind == TokenKind::TypeUnsignedInt8
//            || tokenKind == TokenKind::TypeUnsignedInt16
//            || tokenKind == TokenKind::TypeUnsignedInt32
//            || tokenKind == TokenKind::TypeUnsignedInt64;
        // TODO
        return false;
    }

    bool Classifier::isIntegerType(TokenKind tokenKind) {
        return tokenKind == TokenKind::TypeInt8
            || tokenKind == TokenKind::TypeInt16
            || tokenKind == TokenKind::TypeInt32
            || tokenKind == TokenKind::TypeInt64
            || Classifier::isUnsignedIntegerType(tokenKind);
    }

    bool Classifier::isKeyword(TokenKind tokenKind) {
        return TokenConst::getKeywords().contains(tokenKind);
    }

    bool Classifier::isLiteral(TokenKind tokenKind) {
        return tokenKind == TokenKind::LiteralInteger
            || tokenKind == TokenKind::LiteralDecimal
            || tokenKind == TokenKind::LiteralCharacter
            || tokenKind == TokenKind::LiteralString;
    }

    bool Classifier::isStatement(TokenKind tokenKind, std::optional<TokenKind> nextTokenKind) {
        return Classifier::isBuiltInType(tokenKind)
            || (
                nextTokenKind.has_value()
                    && tokenKind == TokenKind::Identifier
                    && nextTokenKind.value() == TokenKind::SymbolEqual
            )

            || tokenKind == TokenKind::KeywordIf
            || tokenKind == TokenKind::KeywordReturn;
    }
}
