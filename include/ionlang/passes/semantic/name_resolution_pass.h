#pragma once

#include <ionshared/diagnostics/diagnostic.h>
#include <ionshared/tracking/symbol_table.h>
#include <ionlang/misc/helpers.h>
#include <ionlang/passes/pass.h>

namespace ionlang {
    /**
     * Resolves partial constructs which reference
     * undefined symbols at the time by their identifier(s).
     */
    class NameResolutionPass : public Pass {
    private:
        std::list<ionshared::PtrSymbolTable<Construct>> scope;

    public:
        IONSHARED_PASS_ID;

        explicit NameResolutionPass(
            ionshared::Ptr<ionshared::PassContext> context
        );

        void visitModule(ionshared::Ptr<Module> node) override;

        void visitScopeAnchor(ionshared::Ptr<ionshared::Scoped<Construct>> node) override;

        void visitRef(PtrRef<> node) override;

        [[nodiscard]] const std::list<ionshared::PtrSymbolTable<Construct>> &getScope() const;
    };
}
