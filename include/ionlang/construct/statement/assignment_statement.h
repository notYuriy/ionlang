#pragma once

#include <string>
#include <ionshared/misc/helpers.h>
#include <ionlang/construct/pseudo/ref.h>
#include <ionlang/construct/value.h>
#include <ionlang/construct/statement.h>
#include "variable_decl_statement.h"

namespace ionlang {
    class Pass;

    struct AssignmentStatementOpts : StatementOpts {
        PtrRef<VariableDeclStatement> variableDeclStatementRef;

        ionshared::Ptr<Construct> value;
    };

    struct AssignmentStatement : Statement {
        PtrRef<VariableDeclStatement> variableDeclStatementRef;

        ionshared::Ptr<Construct> value;

        explicit AssignmentStatement(const AssignmentStatementOpts &opts);

        void accept(Pass &visitor) override;

        [[nodiscard]] Ast getChildNodes() override;
    };
}
