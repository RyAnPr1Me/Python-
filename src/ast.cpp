#include "ast.h"

namespace pyplusplus {

// Expression accept methods
void LiteralExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void IdentifierExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BinaryExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void UnaryExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void CallExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void AttributeExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void SubscriptExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ListExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void DictExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }

// Statement accept methods
void ExpressionStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void AssignmentStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void IfStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void WhileStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ForStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void FunctionDef::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ReturnStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BreakStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ContinueStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void PassStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }

} // namespace pyplusplus