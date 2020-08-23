#include <ionshared/misc/util.h>
#include <ionir/construct/value/integer_value.h>
#include <ionir/construct/value/char_value.h>
#include <ionir/construct/value/string_value.h>
#include <ionir/construct/value/boolean_value.h>
#include <ionir/construct/function.h>
#include <ionir/construct/global.h>
#include <ionir/misc/inst_builder.h>
#include <ionlang/passes/codegen/ionir_codegen_pass.h>
#include <ionlang/const/notice.h>

namespace ionlang {
    void IonIrCodegenPass::requireModule() {
        if (!ionshared::Util::hasValue(this->moduleBuffer)) {
            throw std::runtime_error("Expected the module buffer to be set, but was null");
        }
    }

    void IonIrCodegenPass::requireFunction() {
        if (!ionshared::Util::hasValue(this->functionBuffer)) {
            throw std::runtime_error("Expected the function buffer to be set, but was null");
        }
    }

    void IonIrCodegenPass::requireBuilder() {
        // Builder must be instantiated.
        if (!this->builderBuffer.has_value()) {
            // Otherwise, throw a runtime error.
            throw std::runtime_error("Expected builder to be instantiated");
        }
    }

    void IonIrCodegenPass::setBuilder(ionshared::Ptr<ionir::BasicBlock> basicBlock) {
        this->builderBuffer = basicBlock->createBuilder();
        this->basicBlockBuffer = basicBlock;
    }

    IonIrCodegenPass::IonIrCodegenPass(ionshared::PtrSymbolTable<ionir::Module> modules)
        : modules(std::move(modules)), constructStack(), typeStack(), moduleBuffer(std::nullopt), functionBuffer(std::nullopt), basicBlockBuffer(std::nullopt), builderBuffer(std::nullopt) {
        //
    }

    IonIrCodegenPass::~IonIrCodegenPass() {
        // TODO
    }

    ionshared::Stack<ionshared::Ptr<ionir::Construct>> IonIrCodegenPass::getConstructStack() const noexcept {
        return this->constructStack;
    }

    ionshared::Stack<ionshared::Ptr<ionir::Type>> IonIrCodegenPass::getTypeStack() const noexcept {
        return this->typeStack;
    }

    ionshared::Ptr<ionshared::SymbolTable<ionshared::Ptr<ionir::Module>>> IonIrCodegenPass::getModules() const {
        return this->modules;
    }

    ionshared::OptPtr<ionir::Module> IonIrCodegenPass::getModuleBuffer() const {
        return this->moduleBuffer;
    }

    bool IonIrCodegenPass::setModuleBuffer(const std::string &id) {
        if (this->modules->contains(id)) {
            this->moduleBuffer = this->modules->lookup(id);

            return true;
        }

        return false;
    }

    void IonIrCodegenPass::visit(ionshared::Ptr<Construct> node) {
        /**
         * Only instruct the node to visit this instance and
         * not its children, since they're already visited by
         * the other member methods.
         */
        node->accept(*this);
    }

    void IonIrCodegenPass::visitModule(ionshared::Ptr<Module> node) {
        this->moduleBuffer = std::make_shared<ionir::Module>(node->getId());

        // Set the module on the modules symbol table.
        this->modules->insert(node->getId(), *this->moduleBuffer);

        // Proceed to visit all the module's children (top-level constructs).
        std::map<std::string, ionshared::Ptr<Construct>> moduleSymbolTable =
            node->getSymbolTable()->unwrap();

        for (const auto &[id, topLevelConstruct] : moduleSymbolTable) {
            this->visit(topLevelConstruct);
        }
    }

    void IonIrCodegenPass::visitFunction(ionshared::Ptr<Function> node) {
        this->requireModule();

        // TODO: Awaiting verification implementation.
//        if (!node->verify()) {
//            throw ionshared::Util::quickError(
//                IONLANG_NOTICE_MISC_VERIFICATION_FAILED,
//                "Function" // TODO: Hard-coded, should be using Util::getConstructName().
//            );
//        }

        std::string ionIrFunctionId = node->getPrototype()->getId();

        if ((*this->moduleBuffer)->getSymbolTable()->contains(ionIrFunctionId)) {
            throw ionshared::Util::quickError(
                IONLANG_NOTICE_FUNCTION_ALREADY_DEFINED,
                ionIrFunctionId
            );
        }

        // Visit the prototype.
        this->visitPrototype(node->getPrototype());

        // Retrieve the resulting function off the stack.
        ionshared::Ptr<ionir::Function> ionIrFunction =
            this->constructStack.pop()->dynamicCast<ionir::Function>();

        // Set the function buffer.
        this->functionBuffer = ionIrFunction;

        // Register the newly created function on the buffer module's symbol table.
        (*this->moduleBuffer)->insertFunction(ionIrFunction);

        // Visit the function's body.
        this->visitFunctionBody(node->getBody());

        // TODO: Verify the resulting LLVM function (through LLVM).

        // Push the function back onto the stack.
        this->constructStack.push(ionIrFunction);
    }

    void IonIrCodegenPass::visitExtern(ionshared::Ptr<Extern> node) {
        if (node->getPrototype() == nullptr) {
            throw std::runtime_error("Unexpected external definition's prototype to be null");
        }

        ionshared::OptPtr<ionir::Function> existingDefinition =
            moduleBuffer->get()->lookupFunction(node->getPrototype()->getId());

        if (ionshared::Util::hasValue(existingDefinition)) {
            throw std::runtime_error("Re-definition of extern not allowed");
        }

        // Visit the prototype.
        this->visitPrototype(node->getPrototype());

        // No need to push the resulting function onto the stack.
    }

    void IonIrCodegenPass::visitPrototype(ionshared::Ptr<Prototype> node) {
        this->requireModule();

        // Retrieve argument count from the argument vector.
        uint32_t argumentCount = node->getArgs()->getItems().getSize();

        // Create the argument buffer vector.
        ionshared::Ptr<ionir::Args> arguments = std::make_shared<ionir::Args>();

        // Attempt to retrieve an existing function.
        ionshared::OptPtr<ionir::Function> ionIrFunction =
            this->moduleBuffer->get()->lookupFunction(node->getId());

        // A function with a matching identifier already exists.
        if (ionshared::Util::hasValue(ionIrFunction)) {
            ionshared::Ptr<ionir::Prototype> ionIrPrototype = ionIrFunction->get()->getPrototype();
            size_t  existingFunctionArgumentCount = ionIrPrototype->getArgs()->getItems().getSize();

            // Function already has a body, disallow re-definition.
            if (!ionIrFunction->get()->getBody()->getSymbolTable()->isEmpty()) {
                // TODO
                throw ionshared::Util::quickError<std::string>(
                    IONLANG_NOTICE_FUNCTION_ALREADY_DEFINED,
                    ionIrPrototype->getId()
                );
            }
            // If the function takes a different number of arguments, reject.
            else if (existingFunctionArgumentCount != argumentCount) {
                throw ionshared::Util::quickError(
                    IONLANG_NOTICE_FUNCTION_REDEFINITION_DIFFERENT_SIG,
                    ionIrPrototype->getId()
                );
            }
        }
        // Otherwise, function will be created.
        else {
            for (uint32_t i = 0; i < argumentCount; ++i) {
                // TODO: Process arguments.
//                arguments.push_back(llvm::Type::getDoubleTy(**this->contextBuffer));
            }

            // Visit and pop the return type.
            this->visitType(node->getReturnType());

            ionshared::Ptr<ionir::Type> ionIrReturnType = this->typeStack.pop();

            // TODO: Support for variable arguments and hard-coded return type.
            // TODO: Args and parent (ionir::Module) in form of IonIR entities.
            ionshared::Ptr<ionir::Prototype> ionIrPrototype =
                std::make_shared<ionir::Prototype>(node->getId(), nullptr, ionIrReturnType, nullptr);

            // Function body will be nullptr until set once it's visited.
            ionIrFunction = std::make_shared<ionir::Function>(ionIrPrototype, nullptr);

            /**
             * Insert the previously created function and register it
             * within the current module buffer.
             */
            this->moduleBuffer->get()->insertFunction(*ionIrFunction);
        }

        uint32_t i = 0;
        auto args = ionIrFunction->get()->getPrototype()->getArgs()->getItems().unwrap();

        for (const auto &arg : args) {
            // TODO: getItems() no longer a vector; cannot index by index, only key.
            // Retrieve the name element from the argument tuple.
            //            std::string name = node->getArgs()->getItems()[i].second;

            // Name the argument.
            //            arg.setName(name);

            // Increment the counter to prepare for next iteration.
            i++;
        }

        this->constructStack.push(*ionIrFunction);
    }

    void IonIrCodegenPass::visitIntegerValue(ionshared::Ptr<IntegerValue> node) {
        ionshared::Ptr<Type> nodeType = node->getType();

        if (nodeType->getTypeKind() != TypeKind::Integer) {
            throw std::runtime_error("Integer value's type must be integer type");
        }

        this->visitIntegerType(nodeType->dynamicCast<IntegerType>());

        ionshared::Ptr<ionir::IntegerType> ionIrIntegerType = this->typeStack.pop()->dynamicCast<ionir::IntegerType>();

        ionshared::Ptr<ionir::IntegerValue> ionIrIntegerValue =
            std::make_shared<ionir::IntegerValue>(ionIrIntegerType, node->getValue());

        this->constructStack.push(ionIrIntegerValue->dynamicCast<ionir::Value<>>());
    }

    void IonIrCodegenPass::visitCharValue(ionshared::Ptr<CharValue> node) {
        ionshared::Ptr<ionir::CharValue> ionIrCharValue =
            std::make_shared<ionir::CharValue>(node->getValue());

        this->constructStack.push(ionIrCharValue->dynamicCast<ionir::Value<>>());
    }

    void IonIrCodegenPass::visitStringValue(ionshared::Ptr<StringValue> node) {
        ionshared::Ptr<ionir::StringValue> ionIrStringValue =
            std::make_shared<ionir::StringValue>(node->getValue());

        this->constructStack.push(ionIrStringValue->dynamicCast<ionir::Value<>>());
    }

    void IonIrCodegenPass::visitBooleanValue(ionshared::Ptr<BooleanValue> node) {
        ionshared::Ptr<ionir::BooleanValue> ionIrBooleanValue =
            std::make_shared<ionir::BooleanValue>(node->getValue());

        this->constructStack.push(ionIrBooleanValue->dynamicCast<ionir::Value<>>());
    }

    void IonIrCodegenPass::visitGlobal(ionshared::Ptr<Global> node) {
        // Module buffer will be used, therefore it must be set.
        this->requireModule();

        this->visitType(node->getType());

        ionshared::Ptr<ionir::Type> type = this->typeStack.pop();
        ionshared::OptPtr<Value<>> nodeValue = node->getValue();
        ionshared::OptPtr<ionir::Value<>> value = std::nullopt;

        // Assign value if applicable.
        if (ionshared::Util::hasValue(nodeValue)) {
            // Visit global variable value.
            this->visitValue(*nodeValue);

            value = this->constructStack.pop()->dynamicCast<ionir::Value<>>();
        }

        ionshared::Ptr<ionir::Global> globalVar =
            std::make_shared<ionir::Global>(type, node->getId(), value);

        this->constructStack.push(globalVar);
    }

    void IonIrCodegenPass::visitType(ionshared::Ptr<Type> node) {
        // Convert type to a pointer if applicable.
        // TODO: Now it's PointerType (soon to be implemented or already).
        //        if (node->getIsPointer()) {
        //            /**
        //             * TODO: Convert type to pointer before passing on
        //             * to explicit handlers, thus saving time and code.
        //             */
        //        }

        switch (node->getTypeKind()) {
            case TypeKind::Void: {
                return this->visitVoidType(node->staticCast<VoidType>());
            }

            case TypeKind::Integer: {
                return this->visitIntegerType(node->staticCast<IntegerType>());
            }

            case TypeKind::String: {
                // TODO

                throw std::runtime_error("Not implemented");
            }

            case TypeKind::UserDefined: {
                // TODO

                throw std::runtime_error("Not implemented");
            }

            default: {
                throw std::runtime_error("Could not identify type kind");
            }
        }
    }

    void IonIrCodegenPass::visitIntegerType(ionshared::Ptr<IntegerType> node) {
        ionir::IntegerKind ionIrIntegerKind;

        /**
         * Create the corresponding IonIR integer type based off the
         * node's integer kind.
         */
        switch (node->getIntegerKind()) {
            case IntegerKind::Int8: {
                ionIrIntegerKind = ionir::IntegerKind::Int8;

                break;
            }

            case IntegerKind::Int16: {
                ionIrIntegerKind = ionir::IntegerKind::Int16;

                break;
            }

            case IntegerKind::Int32: {
                ionIrIntegerKind = ionir::IntegerKind::Int32;

                break;
            }

            case IntegerKind::Int64: {
                ionIrIntegerKind = ionir::IntegerKind::Int64;

                break;
            }

            case IntegerKind::Int128: {
                ionIrIntegerKind = ionir::IntegerKind::Int128;

                break;
            }

            default: {
                throw std::runtime_error("An unrecognized integer kind was provided");
            }
        }

        ionshared::Ptr<ionir::IntegerType> ionIrType =
            std::make_shared<ionir::IntegerType>(ionIrIntegerKind);

        this->typeStack.push(ionIrType);
    }

    void IonIrCodegenPass::visitBooleanType(ionshared::Ptr<BooleanType> node) {
        ionshared::Ptr<ionir::BooleanType> ionIrBooleanType =
            std::make_shared<ionir::BooleanType>();

        this->typeStack.push(ionIrBooleanType);
    }

    void IonIrCodegenPass::visitVoidType(ionshared::Ptr<VoidType> node) {
        ionshared::Ptr<ionir::VoidType> ionIrVoidType =
            std::make_shared<ionir::VoidType>();

        this->typeStack.push(ionIrVoidType);
    }

    void IonIrCodegenPass::visitIfStatement(ionshared::Ptr<IfStatement> node) {
        // TODO: Implement.
        throw std::runtime_error("Not implemented");
    }

    void IonIrCodegenPass::visitReturnStatement(ionshared::Ptr<ReturnStatement> node) {
        this->requireBuilder();

        ionshared::Ptr<ionir::InstBuilder> ionIrInstBuilder = *this->builderBuffer;
        ionshared::OptPtr<ionir::Value<>> ionIrValue = std::nullopt;

        if (node->hasValue()) {
            this->visitValue(*node->getValue());
            ionIrValue = this->constructStack.pop()->dynamicCast<ionir::Value<>>();
        }

        ionshared::Ptr<ionir::ReturnInst> ionIrReturnInst =
            ionIrInstBuilder->createReturn(ionIrValue);

        this->constructStack.push(ionIrReturnInst);
    }

    void IonIrCodegenPass::visitVariableDecl(ionshared::Ptr<VariableDecl> node) {
        this->requireBuilder();

        ionshared::Ptr<ionir::InstBuilder> ionIrInstBuilder = *this->builderBuffer;

        // First, visit the type and create a IonIR alloca inst,  and push it onto the stack.
        this->visitType(node->getType());

        ionshared::Ptr<ionir::Type> ionIrType = this->typeStack.pop();

        ionshared::Ptr<ionir::AllocaInst> ionIrAllocaInst =
            ionIrInstBuilder->createAlloca(node->getId(), ionIrType);

        this->constructStack.push(ionIrAllocaInst);

        // Lastly, then create a IonIR store inst, and push it onto the stack.
        this->visitValue(node->getValue());

        ionshared::Ptr<ionir::Value<>> ionIrValue =
            this->constructStack.pop()->dynamicCast<ionir::Value<>>();

        ionir::PtrRef<ionir::AllocaInst> ionIrRef = std::make_shared<ionir::Ref<ionir::AllocaInst>>(
            node->getId(),

            // TODO: CRITICAL: Provide owner for ref!
            nullptr,

            ionIrAllocaInst
        );

        ionshared::Ptr<ionir::StoreInst> ionIrStoreInst = ionIrInstBuilder->createStore(
            ionIrValue,
            ionIrRef
        );
    }

    void IonIrCodegenPass::visitCallStatement(ionshared::Ptr<CallStatement> node) {
        this->requireBuilder();

        ionshared::Ptr<ionir::InstBuilder> ionIrInstBuilder = *this->builderBuffer;

        // TODO: Continue/finish implementation.
//        ionIrInstBuilder->createCall()

        throw std::runtime_error("Not implemented");
    }
}
