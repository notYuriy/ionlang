#include <ionlang/passes/pass.h>

namespace ionlang {
    Attribute::Attribute(ionshared::Ptr<Construct> parent, std::string id) :
        ConstructWithParent<>(std::move(parent), ConstructKind::Attribute),
        ionshared::Named{std::move(id)} {
        //
    }

    void Attribute::accept(Pass &visitor) {
        visitor.visitAttribute(this->dynamicCast<Attribute>());
    }
}
