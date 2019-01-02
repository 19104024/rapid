/**
 *
 * @file Variable.hpp
 *
 * Program variables (and variables coming from assertion quantifiers)
 */

#ifndef __ProgramVariable__
#define __ProgramVariable__

#include <string>
#include <ostream>

#include "Expression.hpp"

#include "Term.hpp"
#include "Formula.hpp"
#include "Signature.hpp"

#include <iostream>
namespace program {
    
    class Variable
    {
    public:
        Variable(std::string name, bool isConstant, bool isArray) : name(name), isConstant(isConstant), isArray(isArray) {}

        const std::string name;
        const bool isConstant;
        const bool isArray;
        virtual void addSymbolToSignature() const;
        
        bool operator==(const Variable& rhs) const { return (name == rhs.name); }
        bool operator!=(const Variable& rhs) const { return !operator==(rhs); }
        
        std::shared_ptr<const logic::Term> toTerm(std::shared_ptr<const logic::Term> index) const;
        std::shared_ptr<const logic::Term> toTerm(std::shared_ptr<const logic::Term> index, std::shared_ptr<const logic::Term> position) const;
    };

    // hack needed for bison: std::vector has no overload for ostream, but these overloads are needed for bison
    std::ostream& operator<<(std::ostream& ostr, const std::vector< std::shared_ptr<const program::Variable>>& e);
    
    class IntVariableAccess : public IntExpression
    {
    public:
        IntVariableAccess(std::shared_ptr<const Variable> var) : IntExpression(), var(var) {}
        
        const std::shared_ptr<const Variable> var;

        IntExpression::Type type() const override {return IntExpression::Type::IntVariableAccess;}
        
        std::string toString() const override;
        std::shared_ptr<const logic::Term> toTerm(std::shared_ptr<const logic::Term> index) const override;
    };
    
    class IntArrayApplication : public IntExpression
    {
    public:
        IntArrayApplication(std::shared_ptr<const Variable> array, std::shared_ptr<const IntExpression> index) : array(std::move(array)), index(std::move(index))
        {
            assert(this->array != nullptr);
            assert(this->index != nullptr);
        }
        
        const std::shared_ptr<const Variable> array;
        const std::shared_ptr<const IntExpression> index;
        
        IntExpression::Type type() const override {return IntExpression::Type::IntArrayApplication;}

        std::string toString() const override;
        std::shared_ptr<const logic::Term> toTerm(std::shared_ptr<const logic::Term> index, std::shared_ptr<const logic::Term> position) const;
        std::shared_ptr<const logic::Term> toTerm(std::shared_ptr<const logic::Term> i) const override;
    };
}

#endif // __ProgramVariable__
