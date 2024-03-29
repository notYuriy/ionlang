#pragma once

#include <string>
#include <ionshared/misc/named.h>
#include <ionshared/misc/helpers.h>
#include <ionshared/tracking/symbol_table.h>
#include <ionshared/tracking/scoped.h>
#include <ionlang/construct/pseudo/child_construct.h>
#include "ionlang/construct/statement/variable_decl_statement.h"
#include "statement.h"
#include "function.h"

namespace ionlang {
    class Pass;

    class StatementBuilder;

    // TODO: Must be verified to contain a single terminal instruction at the end?
    struct Block : ConstructWithParent<>, ionshared::Scoped<VariableDeclStatement> {
        // TODO: When statements are mutated, the symbol table must be cleared and re-populated.
        std::vector<ionshared::Ptr<Statement>> statements;

        explicit Block(
            ionshared::Ptr<Construct> parent,

            std::vector<ionshared::Ptr<Statement>> statements = {},

            const ionshared::PtrSymbolTable<VariableDeclStatement> &symbolTable =
                ionshared::util::makePtrSymbolTable<VariableDeclStatement>()
        );

        void accept(Pass &visitor) override;

        [[nodiscard]] Ast getChildNodes() override;

        /**
         * Append a statement to the local statement vector, and if
         * applicable, register it on the local symbol table. No
         * relocation is performed, and the statement's parent
         * is left intact.
         */
        void appendStatement(const ionshared::Ptr<Statement> &statement);

        /**
         * Move the statement at the provided order index from this block
         * to another. The statement will be removed from the local vector,
         * and registered on the target block's symbol table.
         */
        bool relocateStatement(size_t orderIndex, ionshared::Ptr<Block> target);

        size_t relocateStatements(
            const ionshared::Ptr<Block> &target,
            size_t from = 0,
            std::optional<size_t> to = std::nullopt
        );

        /**
         * Splits the local basic block, relocating all instructions
         * within the provided range (or all after the starting index if
         * no end index was provided) to a new basic block with the same
         * parent as this local block.
         */
        [[nodiscard]] ionshared::Ptr<Block> slice(
            size_t from,
            std::optional<size_t> to = std::nullopt
        );

        /**
         * Attempt to find the index location of a statement. Returns null
         * if not found.
         */
        [[nodiscard]] std::optional<size_t> locate(ionshared::Ptr<Statement> statement) const;

        [[nodiscard]] ionshared::Ptr<StatementBuilder> createBuilder();

        /**
         * Loops through all statements collecting terminal statements
         * (or expressions) such as function calls or return statements.
         * Search will be performed only on the local symbol table, so
         * nested blocks will be ignored.
         */
        [[nodiscard]] std::vector<ionshared::Ptr<Statement>> findTerminals() const;

        [[nodiscard]] ionshared::OptPtr<Statement> findFirstStatement() noexcept;

        [[nodiscard]] ionshared::OptPtr<Statement> findLastStatement() noexcept;

        [[nodiscard]] bool isFunctionBody();

        [[nodiscard]] ionshared::OptPtr<Function> findParentFunction();
    };
}
