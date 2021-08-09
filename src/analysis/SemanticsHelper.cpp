#include "SemanticsHelper.hpp"

#include <cassert>

#include "Variable.hpp"
#include "Term.hpp"
#include "Theory.hpp"

#include "SymbolDeclarations.hpp"

namespace analysis {
# pragma mark - Methods for generating most used variables

    std::shared_ptr<const logic::LVariable> posVar() {
        return logic::Terms::var(posVarSymbol());
    }

# pragma mark - Methods for generating most used trace terms

    std::shared_ptr<const logic::Term> traceTerm(unsigned traceNumber) {
        return logic::Terms::func(traceSymbol(traceNumber), {});
    }

    std::vector<std::shared_ptr<const logic::Term>> traceTerms(unsigned numberOfTraces) {
        std::vector<std::shared_ptr<const logic::Term>> traces;
        for (unsigned traceNumber = 1; traceNumber < numberOfTraces + 1; traceNumber++) {
            traces.push_back(traceTerm(traceNumber));
        }
        return traces;
    }


# pragma mark - Methods for generating most used timepoint terms and symbols

    std::shared_ptr<const logic::LVariable> iteratorTermForLoop(const program::WhileStatement *whileStatement) {
        assert(whileStatement != nullptr);

        return logic::Terms::var(iteratorSymbol(whileStatement));
    }

    std::shared_ptr<const logic::Term> lastIterationTermForLoop(const program::WhileStatement *whileStatement, unsigned numberOfTraces, std::shared_ptr<const logic::Term> trace) {
        assert(whileStatement != nullptr);
        assert(trace != nullptr);

        auto symbol = lastIterationSymbol(whileStatement, numberOfTraces);
        std::vector<std::shared_ptr<const logic::Term>> subterms;
        for (const auto &loop : *whileStatement->enclosingLoops) {
            subterms.push_back(iteratorTermForLoop(loop));
        }
        if (numberOfTraces > 1) {
            subterms.push_back(trace);
        }
        return logic::Terms::func(symbol, subterms);
    }

    std::shared_ptr<const logic::Term> timepointForNonLoopStatement(const program::Statement *statement) {
        assert(statement != nullptr);
        assert(statement->type() != program::Statement::Type::WhileStatement);

        auto enclosingLoops = *statement->enclosingLoops;
        auto enclosingIteratorTerms = std::vector<std::shared_ptr<const logic::Term>>();
        for (const auto &enclosingLoop : enclosingLoops) {
            auto enclosingIteratorSymbol = iteratorSymbol(enclosingLoop);
            enclosingIteratorTerms.push_back(logic::Terms::var(enclosingIteratorSymbol));
        }

        return logic::Terms::func(locationSymbolForStatement(statement), enclosingIteratorTerms);
    }

    std::shared_ptr<const logic::Term> timepointForLoopStatement(const program::WhileStatement *whileStatement, std::shared_ptr<const logic::Term> innerIteration) {
        assert(whileStatement != nullptr);
        assert(innerIteration != nullptr);

        auto enclosingLoops = *whileStatement->enclosingLoops;
        auto enclosingIteratorTerms = std::vector<std::shared_ptr<const logic::Term>>();
        for (const auto &enclosingLoop : enclosingLoops) {
            auto enclosingIteratorSymbol = iteratorSymbol(enclosingLoop);
            enclosingIteratorTerms.push_back(logic::Terms::var(enclosingIteratorSymbol));
        }
        enclosingIteratorTerms.push_back(innerIteration);
        return logic::Terms::func(locationSymbolForStatement(whileStatement), enclosingIteratorTerms);
    }

    std::shared_ptr<const logic::Term> startTimepointForStatement(const program::Statement *statement) {
        if (statement->type() != program::Statement::Type::WhileStatement) {
            return timepointForNonLoopStatement(statement);
        }
        else {
            auto whileStatement = static_cast<const program::WhileStatement *>(statement);
            return timepointForLoopStatement(whileStatement, logic::Theory::natZero());
        }
    }

    std::vector<std::shared_ptr<const logic::Symbol>> enclosingIteratorsSymbols(const program::Statement *statement) {
        auto enclosingIteratorsSymbols = std::vector<std::shared_ptr<const logic::Symbol>>();
        for (const auto &enclosingLoop : *statement->enclosingLoops) {
            enclosingIteratorsSymbols.push_back(iteratorSymbol(enclosingLoop));
        }
        return enclosingIteratorsSymbols;
    }


# pragma mark - Methods for generating most used terms/predicates denoting program-expressions

    std::shared_ptr<const logic::Term> toTerm(std::shared_ptr<const program::Variable> var, std::shared_ptr<const logic::Term> timePoint, std::shared_ptr<const logic::Term> trace) {
        assert(var != nullptr);
        assert(trace != nullptr);

        assert(!var->isArray);

        std::vector<std::shared_ptr<const logic::Term>> arguments;

        if (!var->isConstant) {
            assert(timePoint != nullptr);
            arguments.push_back(timePoint);
        }
        if (var->numberOfTraces > 1) {
            arguments.push_back(trace);
        }

        if (var->type() == program::ValueType::Bool) {
            return logic::Formulas::predicate(var->name, arguments);
        }
        return logic::Terms::func(var->name, arguments, logic::Sorts::intSort());
    }

    std::shared_ptr<const logic::Term> toTerm(std::shared_ptr<const program::Variable> var, std::shared_ptr<const logic::Term> timePoint, std::shared_ptr<const logic::Term> position, std::shared_ptr<const logic::Term> trace) {
        assert(var != nullptr);
        assert(position != nullptr);
        assert(trace != nullptr);

        assert(var->isArray);

        std::vector<std::shared_ptr<const logic::Term>> arguments;

        if (!var->isConstant) {
            assert(timePoint != nullptr);
            arguments.push_back(timePoint);
        }

        arguments.push_back(position);

        if (var->numberOfTraces > 1) {
            arguments.push_back(trace);
        }

        if (var->type() == program::ValueType::Bool) {
            return logic::Formulas::predicate(var->name, arguments);
        }
        return logic::Terms::func(var->name, arguments, logic::Sorts::intSort());
    }

    std::shared_ptr<const logic::Term> toTerm(std::shared_ptr<const program::Expression> expr, std::shared_ptr<const logic::Term> timePoint, std::shared_ptr<const logic::Term> trace) {
        assert(expr != nullptr);
        assert(timePoint != nullptr);

        if (typeid(*expr) == typeid(program::ArithmeticConstant)) {
            auto castedExpr = std::static_pointer_cast<const program::ArithmeticConstant>(expr);
            return logic::Theory::intConstant(castedExpr->value);
        }
        else if (typeid(*expr) == typeid(program::Addition)) {
            auto castedExpr = std::static_pointer_cast<const program::Addition>(expr);
            return logic::Theory::intAddition(toTerm(castedExpr->child1, timePoint, trace), toTerm(castedExpr->child2, timePoint, trace));
        }
        else if (typeid(*expr) == typeid(program::Subtraction)) {
            auto castedExpr = std::static_pointer_cast<const program::Subtraction>(expr);
            return logic::Theory::intSubtraction(toTerm(castedExpr->child1, timePoint, trace), toTerm(castedExpr->child2, timePoint, trace));
        }
        else if (typeid(*expr) == typeid(program::Modulo)) {
            auto castedExpr = std::static_pointer_cast<const program::Modulo>(expr);
            return logic::Theory::intModulo(toTerm(castedExpr->child1, timePoint, trace), toTerm(castedExpr->child2, timePoint, trace));
        }
        else if (typeid(*expr) == typeid(program::Multiplication)) {
            auto castedExpr = std::static_pointer_cast<const program::Multiplication>(expr);
            return logic::Theory::intMultiplication(toTerm(castedExpr->child1, timePoint, trace), toTerm(castedExpr->child2, timePoint, trace));
        }
        else if (typeid(*expr) == typeid(program::VariableAccess)) {
            auto castedExpr = std::static_pointer_cast<const program::VariableAccess>(expr);
            return toTerm(castedExpr->var, timePoint, trace);
        }
        else if (typeid(*expr) == typeid(program::ArrayApplication)) {
            auto castedExpr = std::static_pointer_cast<const program::ArrayApplication>(expr);
            return toTerm(castedExpr->array, timePoint, toTerm(castedExpr->index, timePoint, trace), trace);
        }
        else if (typeid(*expr) == typeid(program::BooleanConstant)) {
            auto castedExpr = std::static_pointer_cast<const program::BooleanConstant>(expr);
            return castedExpr->value ? logic::Theory::boolTrue() : logic::Theory::boolFalse();
        }
        else if (typeid(*expr) == typeid(program::BooleanAnd)) {
            auto castedExpr = std::static_pointer_cast<const program::BooleanAnd>(expr);
            return logic::Formulas::conjunction({toTerm(castedExpr->child1, timePoint, trace), toTerm(castedExpr->child2, timePoint, trace)});
        }
        else if (typeid(*expr) == typeid(program::BooleanOr)) {
            auto castedExpr = std::static_pointer_cast<const program::BooleanOr>(expr);
            return logic::Formulas::disjunction({toTerm(castedExpr->child1, timePoint, trace), toTerm(castedExpr->child2, timePoint, trace)});
        }
        else if (typeid(*expr) == typeid(program::BooleanNot)) {
            auto castedExpr = std::static_pointer_cast<const program::BooleanNot>(expr);
            return logic::Formulas::negation(toTerm(castedExpr->child, timePoint, trace));
        }
        else if (typeid(*expr) == typeid(program::ArithmeticComparison)) {
            auto castedExpr = std::static_pointer_cast<const program::ArithmeticComparison>(expr);
            switch (castedExpr->kind) {
                case program::ArithmeticComparison::Kind::GT:
                    return logic::Theory::intGreater(toTerm(castedExpr->child1, timePoint, trace), toTerm(castedExpr->child2, timePoint, trace));
                case program::ArithmeticComparison::Kind::GE:
                    return logic::Theory::intGreaterEqual(toTerm(castedExpr->child1, timePoint, trace), toTerm(castedExpr->child2, timePoint, trace));
                case program::ArithmeticComparison::Kind::LT:
                    return logic::Theory::intLess(toTerm(castedExpr->child1, timePoint, trace), toTerm(castedExpr->child2, timePoint, trace));
                case program::ArithmeticComparison::Kind::LE:
                    return logic::Theory::intLessEqual(toTerm(castedExpr->child1, timePoint, trace), toTerm(castedExpr->child2, timePoint, trace));
                case program::ArithmeticComparison::Kind::EQ:
                    return logic::Formulas::equality(toTerm(castedExpr->child1, timePoint, trace), toTerm(castedExpr->child2, timePoint, trace));
            }
        }
        assert(0);
    }

    std::shared_ptr<const logic::Term> varEqual(std::shared_ptr<const program::Variable> v, std::shared_ptr<const logic::Term> timePoint1, std::shared_ptr<const logic::Term> timePoint2, std::shared_ptr<const logic::Term> trace) {
        if (!v->isArray) {
            return
                logic::Formulas::equality(
                    toTerm(v,timePoint1,trace),
                    toTerm(v,timePoint2,trace)
                );
        }
        else {
            auto posSymbol = posVarSymbol();
            auto pos = posVar();
            return
                logic::Formulas::universal({posSymbol},
                    logic::Formulas::equality(
                        toTerm(v,timePoint1,pos,trace),
                        toTerm(v,timePoint2,pos,trace)
                    )
                );
        }
    }

    std::shared_ptr<const logic::Term> allVarEqual(const std::vector<std::shared_ptr<const program::Variable>>& activeVars, std::shared_ptr<const logic::Term> timePoint1, std::shared_ptr<const logic::Term> timePoint2, std::shared_ptr<const logic::Term> trace, std::string label) {
        std::vector<std::shared_ptr<const logic::Term>> conjuncts;
        for (const auto& var : activeVars) {
            if (!var->isConstant) {
                conjuncts.push_back(varEqual(var, timePoint1, timePoint2, trace));
            }
        }
        return logic::Formulas::conjunction(conjuncts, label);
    }
}
