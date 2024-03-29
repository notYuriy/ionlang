#include <ionlang/passes/pass.h>
#include <ionlang/type_system/type_factory.h>

namespace ionlang {
    CharLiteral::CharLiteral(char value) :
        Value(ValueKind::Character, type_factory::typeChar()),
        value(value) {
        //
    }

    void CharLiteral::accept(Pass &visitor) {
        visitor.visitCharLiteral(this->dynamicCast<CharLiteral>());
    }
}
