#ifndef __Term__
#define __Term__

#include <cassert>
#include <initializer_list>
#include <list>
#include <ostream>
#include <string>
#include <vector>
#include "Signature.hpp"
#include "Sort.hpp"

namespace logic {
    
    class LVariable;
    
    class Term
    {
    public:
        Term(const Symbol* symbol) : symbol(symbol) {}
        
        const Symbol* symbol;
        
        virtual std::string toSMTLIB() const = 0;
        virtual std::string prettyString() const = 0;
    };
    
    class LVariable : public Term
    {
        friend class Terms;
        
        LVariable(const Symbol* symbol) : Term(symbol), id(freshId++){}

    public:
        const unsigned id;

        std::string toSMTLIB() const override;
        virtual std::string prettyString() const override;
        
        static unsigned freshId;
    };
    
    bool compareLVarPointers(const LVariable* p1, const LVariable* p2);
    bool eqLVarPointers(const LVariable* p1, const LVariable* p2);
    
    class FuncTerm : public Term
    {
        friend class Terms;
        FuncTerm(const Symbol* symbol, std::initializer_list<std::shared_ptr<const Term>> subterms) : Term(symbol), subterms(subterms)
        {
            assert(symbol->argSorts.size() == subterms.size());
            for (int i=0; i < symbol->argSorts.size(); ++i)
            {
                assert(symbol->argSorts[i] == this->subterms[i]->symbol->rngSort);
            }
        }
        
    public:
        const std::vector<std::shared_ptr<const Term>> subterms;
        
        std::string toSMTLIB() const override;
        virtual std::string prettyString() const override;
    };
    
    // taking the FOOL approach, predicates are alse terms
    class PredTerm : public Term
    {
        friend class Terms;
        PredTerm(const Symbol* symbol, std::initializer_list<std::shared_ptr<const Term>> subterms) : Term(symbol), subterms(subterms)
        {
            assert(symbol->argSorts.size() == subterms.size());
            for (int i=0; i < symbol->argSorts.size(); ++i)
            {
                assert(symbol->argSorts[i] == this->subterms[i]->symbol->rngSort);
            }
        }

    public:
        const std::vector<std::shared_ptr<const Term>> subterms;
        
        std::string toSMTLIB() const override;
        virtual std::string prettyString() const override;
    };
    
    inline std::ostream& operator<<(std::ostream& ostr, const Term& e) { ostr << e.toSMTLIB(); return ostr; }

    
# pragma mark - Terms
    
    // We use Terms as a manager-class for Term-instances
    class Terms
    {
    public:

        // construct new terms
        static std::shared_ptr<const LVariable> lVariable(const Sort* s, const std::string name);
        static std::shared_ptr<const FuncTerm> funcTerm(const Sort* sort, std::string name, std::initializer_list<std::shared_ptr<const Term>> subterms, bool noDeclaration=false);
        static std::shared_ptr<const PredTerm> predTerm(std::string name, std::initializer_list<std::shared_ptr<const Term>> subterms, bool noDeclaration=false);
    };
}
#endif

