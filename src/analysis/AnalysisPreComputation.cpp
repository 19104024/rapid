#include "AnalysisPreComputation.hpp"

#include <cassert>
#include <memory>

#include "SemanticsHelper.hpp"
#include "SymbolDeclarations.hpp"
#include "Theory.hpp"

namespace analysis {
EndTimePointMap AnalysisPreComputation::computeEndTimePointMap(
    const program::Program& program) {
  EndTimePointMap endTimePointMap;
  for (const auto& function : program.functions) {
    // for each statement except the first, set the end-location of the previous
    // statement to the begin-location of this statement
    for (int i = 1; i < function->statements.size(); ++i) {
      auto lastStatement = function->statements[i - 1].get();
      auto nextTimepoint =
          startTimepointForStatement(function->statements[i].get());
      addEndTimePointForStatement(lastStatement, nextTimepoint,
                                  endTimePointMap);
    }
    // for the last statement, set the end-location to be the end-location of
    // the function.
    auto lastStatement = function->statements.back().get();
    auto nextTimepoint =
        logic::Terms::func(locationSymbolEndLocation(function.get()), {});
    addEndTimePointForStatement(lastStatement, nextTimepoint, endTimePointMap);
  }
  return endTimePointMap;
}

void AnalysisPreComputation::addEndTimePointForStatement(
    const program::Statement* statement,
    const std::shared_ptr<const logic::Term> nextTimepoint,
    EndTimePointMap& endTimePointMap) {
  // for an ifElse statement, set endTimepoint and recurse
  if (statement->type() == program::Statement::Type::IfElse) {
    endTimePointMap[statement] = nextTimepoint;
    auto castedStatement = static_cast<const program::IfElse*>(statement);
    addEndTimePointForIfElseStatement(castedStatement, nextTimepoint,
                                      endTimePointMap);
  }
  // for a while statement, set endTimepoint and recurse
  else if (statement->type() == program::Statement::Type::WhileStatement) {
    endTimePointMap[statement] = nextTimepoint;

    auto castedStatement =
        static_cast<const program::WhileStatement*>(statement);
    addEndTimePointForWhileStatement(castedStatement, nextTimepoint,
                                     endTimePointMap);
  }
  // set endTimepoint for atomic statement
  else {
    assert(statement->type() == program::Statement::Type::IntAssignment ||
           statement->type() == program::Statement::Type::SkipStatement);
    endTimePointMap[statement] = nextTimepoint;
  }
}

void AnalysisPreComputation::addEndTimePointForIfElseStatement(
    const program::IfElse* ifElse,
    const std::shared_ptr<const logic::Term> nextTimepoint,
    EndTimePointMap& endTimePointMap) {
  // for each statement in the left branch except the first, set the
  // end-location of the previous statement to the begin-location of this
  // statement
  for (int i = 1; i < ifElse->ifStatements.size(); ++i) {
    auto lastStatement = ifElse->ifStatements[i - 1].get();
    auto nextTimepointForStatement =
        startTimepointForStatement(ifElse->ifStatements[i].get());
    addEndTimePointForStatement(lastStatement, nextTimepointForStatement,
                                endTimePointMap);
  }
  // for each statement in the right branch except the first, set the
  // end-location of the previous statement to the begin-location of this
  // statement
  for (int i = 1; i < ifElse->elseStatements.size(); ++i) {
    auto lastStatement = ifElse->elseStatements[i - 1].get();
    auto nextTimepointForStatement =
        startTimepointForStatement(ifElse->elseStatements[i].get());
    addEndTimePointForStatement(lastStatement, nextTimepointForStatement,
                                endTimePointMap);
  }
  // for both left and right branch, set the endTimePoint of the
  // ifElse-statement as endTimePoint for the last statement
  auto lastStatementLeft = ifElse->ifStatements.back().get();
  auto lastStatementRight = ifElse->elseStatements.back().get();
  addEndTimePointForStatement(lastStatementLeft, nextTimepoint,
                              endTimePointMap);
  addEndTimePointForStatement(lastStatementRight, nextTimepoint,
                              endTimePointMap);
}

void AnalysisPreComputation::addEndTimePointForWhileStatement(
    const program::WhileStatement* whileStatement,
    const std::shared_ptr<const logic::Term> nextTimepoint,
    EndTimePointMap& endTimePointMap) {
  // for each statement in the body except the first, set the end-location of
  // the previous statement to the begin-location of this statement
  for (int i = 1; i < whileStatement->bodyStatements.size(); ++i) {
    auto lastStatement = whileStatement->bodyStatements[i - 1].get();
    auto nextTimepointForStatement =
        startTimepointForStatement(whileStatement->bodyStatements[i].get());
    addEndTimePointForStatement(lastStatement, nextTimepointForStatement,
                                endTimePointMap);
  }

  // for the last statement in the body, set as end-timepoint the start-location
  // of the while-statement in the next iteration
  auto lastStatement = whileStatement->bodyStatements.back().get();
  auto nextTimepointForStatement = timepointForLoopStatement(
      whileStatement,
      logic::Theory::natSucc(iteratorTermForLoop(whileStatement)));
  addEndTimePointForStatement(lastStatement, nextTimepointForStatement,
                              endTimePointMap);
}

std::unordered_set<std::shared_ptr<const program::Variable>>
AnalysisPreComputation::computeAssignedVars(
    const program::Statement* statement) {
  std::unordered_set<std::shared_ptr<const program::Variable>> assignedVars;

  switch (statement->type()) {
    case program::Statement::Type::IntAssignment: {
      auto castedStatement =
          static_cast<const program::IntAssignment*>(statement);
      // add variable on lhs to assignedVars, independently from whether those
      // vars are simple ones or arrays.
      if (castedStatement->lhs->type() ==
          program::IntExpression::Type::IntVariableAccess) {
        auto access = static_cast<const program::IntVariableAccess*>(
            castedStatement->lhs.get());
        assignedVars.insert(access->var);
      } else {
        assert(castedStatement->lhs->type() ==
               program::IntExpression::Type::IntArrayApplication);
        auto arrayAccess = static_cast<const program::IntArrayApplication*>(
            castedStatement->lhs.get());
        assignedVars.insert(arrayAccess->array);
      }
      break;
    }
    case program::Statement::Type::IfElse: {
      auto castedStatement = static_cast<const program::IfElse*>(statement);
      // collect assignedVars from both branches
      for (const auto& statement : castedStatement->ifStatements) {
        auto res = computeAssignedVars(statement.get());
        assignedVars.insert(res.begin(), res.end());
      }
      for (const auto& statement : castedStatement->elseStatements) {
        auto res = computeAssignedVars(statement.get());
        assignedVars.insert(res.begin(), res.end());
      }
      break;
    }
    case program::Statement::Type::WhileStatement: {
      auto castedStatement =
          static_cast<const program::WhileStatement*>(statement);
      // collect assignedVars from body
      for (const auto& statement : castedStatement->bodyStatements) {
        auto res = computeAssignedVars(statement.get());
        assignedVars.insert(res.begin(), res.end());
      }
      break;
    }
    case program::Statement::Type::SkipStatement: {
      break;
    }
  }
  return assignedVars;
}

void AnalysisPreComputation::computeVariablesContainedInLoopCondition(
    std::shared_ptr<const program::BoolExpression> expr,
    std::unordered_set<std::shared_ptr<const program::Variable>>& variables) {
  assert(expr != nullptr);
  switch (expr->type()) {
    case program::BoolExpression::Type::BooleanAnd: {
      auto castedExpr =
          std::static_pointer_cast<const program::BooleanAnd>(expr);
      computeVariablesContainedInLoopCondition(castedExpr->child1, variables);
      computeVariablesContainedInLoopCondition(castedExpr->child2, variables);
      break;
    }
    case program::BoolExpression::Type::BooleanOr: {
      auto castedExpr =
          std::static_pointer_cast<const program::BooleanOr>(expr);
      computeVariablesContainedInLoopCondition(castedExpr->child1, variables);
      computeVariablesContainedInLoopCondition(castedExpr->child2, variables);
      break;
    }
    case program::BoolExpression::Type::BooleanNot: {
      auto castedExpr =
          std::static_pointer_cast<const program::BooleanNot>(expr);
      computeVariablesContainedInLoopCondition(castedExpr->child, variables);
      break;
    }
    case program::BoolExpression::Type::ArithmeticComparison: {
      auto castedExpr =
          std::static_pointer_cast<const program::ArithmeticComparison>(expr);
      computeVariablesContainedInLoopCondition(castedExpr->child1, variables);
      computeVariablesContainedInLoopCondition(castedExpr->child2, variables);
      break;
    }
    case program::BoolExpression::Type::BooleanConstant: {
      // do nothing
      break;
    }
  }
}

void AnalysisPreComputation::computeVariablesContainedInLoopCondition(
    std::shared_ptr<const program::IntExpression> expr,
    std::unordered_set<std::shared_ptr<const program::Variable>>& variables) {
  assert(expr != nullptr);
  switch (expr->type()) {
    case program::IntExpression::Type::Addition: {
      auto castedExpr = std::static_pointer_cast<const program::Addition>(expr);
      computeVariablesContainedInLoopCondition(castedExpr->summand1, variables);
      computeVariablesContainedInLoopCondition(castedExpr->summand2, variables);
      break;
    }
    case program::IntExpression::Type::Subtraction: {
      auto castedExpr =
          std::static_pointer_cast<const program::Subtraction>(expr);
      computeVariablesContainedInLoopCondition(castedExpr->child1, variables);
      computeVariablesContainedInLoopCondition(castedExpr->child2, variables);
      break;
    }
    case program::IntExpression::Type::Multiplication: {
      auto castedExpr =
          std::static_pointer_cast<const program::Multiplication>(expr);
      computeVariablesContainedInLoopCondition(castedExpr->factor1, variables);
      computeVariablesContainedInLoopCondition(castedExpr->factor2, variables);
      break;
    }
    case program::IntExpression::Type::Modulo: {
      auto castedExpr = std::static_pointer_cast<const program::Modulo>(expr);
      computeVariablesContainedInLoopCondition(castedExpr->child1, variables);
      computeVariablesContainedInLoopCondition(castedExpr->child2, variables);
      break;
    }
    case program::IntExpression::Type::IntVariableAccess: {
      auto castedExpr =
          std::static_pointer_cast<const program::IntVariableAccess>(expr);
      variables.insert(castedExpr->var);
      break;
    }
    case program::IntExpression::Type::IntArrayApplication: {
      auto castedExpr =
          std::static_pointer_cast<const program::IntArrayApplication>(expr);
      variables.insert(castedExpr->array);
      computeVariablesContainedInLoopCondition(castedExpr->index, variables);
      break;
    }
    case program::IntExpression::Type::ArithmeticConstant: {
      // do nothing
      break;
    }
  }
}
}  // namespace analysis
