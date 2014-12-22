#include <gtest/gtest.h>

#include <parser.h>

#include <map>
#include <string>
#include <tuple>

#include <boost/variant/get.hpp>

using namespace mhc;
using namespace parser;
using namespace std;



TEST(ASTTest, DataType) {
	// Taken from Real World Haskell, Ch. 3
	const auto input =
		"data BookInfo = Book Int String [String]"
		"                deriving (Show, Test)";

	base_expr_node root;
	EXPECT_TRUE(parse(input, root));

	const auto expr = boost::get<base_expr>(&root);
	EXPECT_TRUE(expr != nullptr);

	// We expect to have our func_expr in this root node
	EXPECT_EQ(1, expr->children.size());
	const auto exprF = boost::get<string>(&expr->children[0]);
	EXPECT_TRUE(exprF != nullptr);

	EXPECT_EQ("", *exprF);
}


#if 0
TEST(ASTTest, BasicFunction) {
	const auto testProgram =
		"marklar main() {"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	EXPECT_TRUE(expr != nullptr);
	
	// We expect to have our func_expr in this root node
	EXPECT_EQ(1, expr->children.size());
	func_expr* exprF = boost::get<func_expr>(&expr->children[0]);
	EXPECT_TRUE(exprF != nullptr);

	// Check the function name
	EXPECT_EQ("main", exprF->functionName);
}

TEST(ASTTest, FunctionSingleDecl) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 0;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	EXPECT_TRUE(expr != nullptr);
	func_expr* exprF = boost::get<func_expr>(&expr->children[0]);
	EXPECT_TRUE(exprF != nullptr);

	// Check declarations
	EXPECT_EQ(1, exprF->declarations.size());
	decl_expr* decl = boost::get<decl_expr>(&exprF->declarations[0]);
	EXPECT_TRUE(decl != nullptr);

	EXPECT_EQ("i", decl->declName);

	binary_op* opVal = boost::get<binary_op>(&decl->val);
	EXPECT_TRUE(opVal != nullptr);

	string* opValStr = boost::get<string>(&opVal->lhs);
	EXPECT_TRUE(opValStr != nullptr);
	EXPECT_EQ("0", *opValStr);
}

TEST(ASTTest, FunctionMultiDecl) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 0;"
		"  marklar j = 1;"
		"  marklar k = 2;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	EXPECT_NE(nullptr, expr);

	func_expr* exprF = boost::get<func_expr>(&expr->children[0]);
	EXPECT_NE(nullptr, exprF);

	EXPECT_EQ(3, exprF->declarations.size());

	const map<string, string> expectedNameVals = {
		{"i", "0"},
		{"j", "1"},
		{"k", "2"}
	};

	for (auto& funcDecl : exprF->declarations) {
		// Check declarations
		decl_expr* decl = boost::get<decl_expr>(&funcDecl);
		EXPECT_TRUE(decl != nullptr);

		const auto itr = expectedNameVals.find(decl->declName);
		EXPECT_TRUE(itr != expectedNameVals.end());

		EXPECT_EQ(itr->first, decl->declName);

		binary_op* valOp = boost::get<binary_op>(&decl->val);
		EXPECT_NE(nullptr, valOp);

		string* lhsVal = boost::get<string>(&valOp->lhs);
		EXPECT_NE(nullptr, lhsVal);
		EXPECT_EQ(itr->second, *lhsVal);
	}
}

TEST(ASTTest, FunctionDeclAssign) {
	const auto testProgram =
		"marklar main() {"
		"  marklar r = 1 + 2;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	func_expr* exprF = boost::get<func_expr>(&expr->children[0]);

	// Check declarations
	EXPECT_EQ(1, exprF->declarations.size());
	decl_expr* decl = boost::get<decl_expr>(&exprF->declarations[0]);
	EXPECT_TRUE(decl != nullptr);

	EXPECT_EQ("r", decl->declName);

	binary_op* opExpr = boost::get<binary_op>(&decl->val);
	EXPECT_TRUE(opExpr != nullptr);
	EXPECT_EQ(1, opExpr->operation.size());

	// Check decl value
	EXPECT_EQ("+", opExpr->operation[0].op);

	string* rhsVal = boost::get<string>(&opExpr->operation[0].rhs);
	EXPECT_EQ("2", *rhsVal);
}

TEST(ASTTest, FunctionMultiDeclAssign) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 1 + 2;"
		"  marklar j = i + 2;"
		"  marklar k = i + j;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	func_expr* exprF = boost::get<func_expr>(&expr->children[0]);

	// Check declarations
	EXPECT_EQ(3, exprF->declarations.size());
	decl_expr* decl = boost::get<decl_expr>(&exprF->declarations[0]);
	EXPECT_TRUE(decl != nullptr);

	EXPECT_EQ("i", decl->declName);

	const vector<tuple<string, string, string, string>> expectedNameVals = {
		make_tuple("i", "1", "+", "2"),
		make_tuple("j", "i", "+", "2"),
		make_tuple("k", "i", "+", "j"),
	};

	int index = 0;
	for (auto& funcDecl : exprF->declarations) {
		// Check declarations
		decl_expr* decl = boost::get<decl_expr>(&funcDecl);
		EXPECT_TRUE(decl != nullptr);

		const auto expectedValues = expectedNameVals[index];

		EXPECT_EQ(get<0>(expectedValues), decl->declName);

		binary_op* opExpr = boost::get<binary_op>(&decl->val);
		EXPECT_TRUE(opExpr != nullptr);

		EXPECT_EQ(1, opExpr->operation.size());

		string* lhsVal = boost::get<string>(&opExpr->lhs);
		EXPECT_EQ(get<1>(expectedValues), *lhsVal);

		// Check decl value
		EXPECT_EQ(get<2>(expectedValues), opExpr->operation[0].op);

		string* rhsVal = boost::get<string>(&opExpr->operation[0].rhs);
		EXPECT_EQ(get<3>(expectedValues), *rhsVal);

		index += 1;
	}
}

TEST(ASTTest, FunctionReturn) {
	const auto testProgram =
		"marklar main() {"
		"  return 1;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	func_expr* exprF = boost::get<func_expr>(&expr->children[0]);
	EXPECT_TRUE(exprF != nullptr);

	EXPECT_EQ(0, exprF->declarations.size());
	EXPECT_EQ(1, exprF->expressions.size());

	return_expr* exprR = boost::get<return_expr>(&exprF->expressions[0]);
	EXPECT_TRUE(exprR != nullptr);

	binary_op* exprRval = boost::get<binary_op>(&exprR->ret);
	EXPECT_NE(nullptr, exprRval);

	string* exprRvalStr = boost::get<string>(&exprRval->lhs);
	EXPECT_NE(nullptr, exprRvalStr);
	EXPECT_EQ("1", *exprRvalStr);
}

TEST(ASTTest, FunctionReturnComplex) {
	const auto testProgram =
		"marklar main() {"
		"  return a + b + c + 0 + 1 + d;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	func_expr* exprF = boost::get<func_expr>(&expr->children[0]);
	EXPECT_TRUE(exprF != nullptr);

	EXPECT_EQ(0, exprF->declarations.size());
	EXPECT_EQ(1, exprF->expressions.size());

	return_expr* exprR = boost::get<return_expr>(&exprF->expressions[0]);
	EXPECT_TRUE(exprR != nullptr);
}

TEST(ASTTest, MultipleFunction) {
	const auto testProgram =
		"marklar foo() {}"
		"marklar main() {}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	EXPECT_EQ(2, expr->children.size());

	func_expr* exprF_foo = boost::get<func_expr>(&expr->children[0]);
	EXPECT_TRUE(exprF_foo != nullptr);
	EXPECT_EQ("foo", exprF_foo->functionName);

	func_expr* exprF_main = boost::get<func_expr>(&expr->children[1]);
	EXPECT_TRUE(exprF_main != nullptr);
	EXPECT_EQ("main", exprF_main->functionName);
}

TEST(ASTTest, MultipleComplexFunction) {
	const auto testProgram =
		"marklar bar() {"
		"  marklar a = 0;"
		"  marklar b = 2;"
		"  return a + b + 0;"
		"}"
		"marklar foo() {"
		"  marklar a = 0;"
		"  return a + 0;"
		"}"
		"marklar main() {"
		"  return 0 + 1;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	EXPECT_EQ(3, expr->children.size());
}

TEST(ASTTest, FunctionArgs) {
	const auto testProgram =
		"marklar main(marklar a, marklar b) {"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	EXPECT_TRUE(expr != nullptr);

	func_expr* exprF = boost::get<func_expr>(&expr->children[0]);
	EXPECT_TRUE(exprF != nullptr);

	EXPECT_EQ(2, exprF->args.size());
	EXPECT_EQ("a", exprF->args[0]);
	EXPECT_EQ("b", exprF->args[1]);
}

TEST(ASTTest, FunctionCall) {
	const auto testProgram =
		"marklar foo() {}"
		"marklar main() {"
		"  foo();"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	EXPECT_TRUE(expr != nullptr);

	func_expr* exprF_foo = boost::get<func_expr>(&expr->children[0]);
	EXPECT_TRUE(exprF_foo != nullptr);

	func_expr* exprF_main = boost::get<func_expr>(&expr->children[1]);
	EXPECT_TRUE(exprF_main != nullptr);
	EXPECT_EQ(1, exprF_main->expressions.size());

	call_expr* callExpr = boost::get<call_expr>(&exprF_main->expressions[0]);
	EXPECT_TRUE(callExpr != nullptr);

	EXPECT_EQ("foo", callExpr->funcName);
	EXPECT_EQ(0, callExpr->values.size());
}

TEST(ASTTest, FunctionCallArgs) {
	const auto testProgram =
		"marklar foo(marklar a) {"
		"  return a;"
		"}"
		"marklar main() {"
		"  return foo(45);"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	EXPECT_TRUE(expr != nullptr);

	func_expr* exprF_main = boost::get<func_expr>(&expr->children[1]);
	EXPECT_TRUE(exprF_main != nullptr);

	return_expr* retExpr = boost::get<return_expr>(&exprF_main->expressions[0]);
	EXPECT_TRUE(retExpr != nullptr);

	call_expr* callExpr = boost::get<call_expr>(&retExpr->ret);
	EXPECT_TRUE(callExpr != nullptr);

	EXPECT_EQ("foo", callExpr->funcName);
	EXPECT_EQ(1, callExpr->values.size());

	binary_op* val = boost::get<binary_op>(&callExpr->values[0]);
	EXPECT_TRUE(val != nullptr);

	string* strVal = boost::get<string>(&val->lhs);
	EXPECT_TRUE(strVal != nullptr);
	EXPECT_EQ("45", *strVal);
}

TEST(ASTTest, FunctionCallArgsComplex) {
	const auto testProgram =
		"marklar bar(marklar a, marklar b) {}"
		"marklar foo(marklar a) {"
		"  return bar(a, 5);"
		"}"
		"marklar main() {"
		"  return foo(45);"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	EXPECT_NE(nullptr, expr);

	func_expr* exprF_main = boost::get<func_expr>(&expr->children[1]);
	EXPECT_NE(nullptr, exprF_main);
	
	return_expr* exprR = boost::get<return_expr>(&exprF_main->expressions[0]);
	EXPECT_TRUE(exprR != nullptr);

	call_expr* callExpr = boost::get<call_expr>(&exprR->ret);
	EXPECT_TRUE(callExpr != nullptr);

	EXPECT_EQ("bar", callExpr->funcName);
	EXPECT_EQ(2, callExpr->values.size());

	binary_op* val1 = boost::get<binary_op>(&callExpr->values[0]);
	EXPECT_NE(nullptr, val1);

	string* val1Str = boost::get<string>(&val1->lhs);
	EXPECT_NE(nullptr, val1Str);
	EXPECT_EQ("a", *val1Str);

	binary_op* val2 = boost::get<binary_op>(&callExpr->values[1]);
	EXPECT_NE(nullptr, val2);

	string* val2Str = boost::get<string>(&val2->lhs);
	EXPECT_NE(nullptr, val2Str);
	EXPECT_EQ("5", *val2Str);
}

TEST(ASTTest, FunctionIfStmt) {
	const auto testProgram =
		"marklar main() {"
		"  if (i < 4) {"
		"  }"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	func_expr* exprF_main = boost::get<func_expr>(&expr->children[0]);

	if_expr* exprIf = boost::get<if_expr>(&exprF_main->expressions[0]);
	EXPECT_TRUE(exprIf != nullptr);

	// Check the condition
	string* exprIfOpLhs = boost::get<string>(&exprIf->condition.lhs);
	EXPECT_TRUE(exprIfOpLhs != nullptr);
	EXPECT_EQ("i", *exprIfOpLhs);

	EXPECT_EQ(1, exprIf->condition.operation.size());
	EXPECT_EQ("<", exprIf->condition.operation[0].op);

	string* exprIfOpRhs = boost::get<string>(&exprIf->condition.operation[0].rhs);
	EXPECT_TRUE(exprIfOpRhs != nullptr);
	EXPECT_EQ("4", *exprIfOpRhs);
}

TEST(ASTTest, FunctionIfElseStmt) {
	const auto testProgram =
		"marklar main() {"
		"  if (i < 4) {"
		"    marklar j = 0;"
		"  } else {"
		"    marklar k = 0;"
		"  }"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	func_expr* exprF_main = boost::get<func_expr>(&expr->children[0]);

	if_expr* exprIf = boost::get<if_expr>(&exprF_main->expressions[0]);
	EXPECT_TRUE(exprIf != nullptr);

	EXPECT_EQ(1, exprIf->thenBranch.size());
	EXPECT_EQ(1, exprIf->elseBranch.size());
}

TEST(ASTTest, Assignment) {
	const auto testProgram =
		"marklar main() {"
		"  marklar a = 3;"
		"  a = a + 1;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	func_expr* exprF_main = boost::get<func_expr>(&expr->children[0]);

	// Check the assignment 
	EXPECT_EQ(1, exprF_main->declarations.size());
	EXPECT_EQ(1, exprF_main->expressions.size());

	var_assign* varAssign = boost::get<var_assign>(&exprF_main->expressions[0]);
	EXPECT_TRUE(varAssign != nullptr);

	EXPECT_EQ("a", varAssign->varName);
	
	binary_op* exprRhs = boost::get<binary_op>(&varAssign->varRhs);
	EXPECT_TRUE(exprRhs != nullptr);

	string* exprRhs_Lhs = boost::get<string>(&exprRhs->lhs);
	EXPECT_TRUE(exprRhs_Lhs != nullptr);
	EXPECT_EQ("a", *exprRhs_Lhs);

	EXPECT_EQ(1, exprRhs->operation.size());
	EXPECT_EQ("+", exprRhs->operation[0].op);

	string* exprRhs_Rhs = boost::get<string>(&exprRhs->operation[0].rhs);
	EXPECT_TRUE(exprRhs_Rhs != nullptr);
	EXPECT_EQ("1", *exprRhs_Rhs);
}

TEST(ASTTest, WhileStmt) {
	const auto testProgram =
		"marklar main() {"
		"  while (i < 4) {"
		"  }"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	func_expr* exprF_main = boost::get<func_expr>(&expr->children[0]);

	while_loop* exprLoop = boost::get<while_loop>(&exprF_main->expressions[0]);
	EXPECT_TRUE(exprLoop != nullptr);

	// Check the condition
	string* exprLoopOpLhs = boost::get<string>(&exprLoop->condition.lhs);
	EXPECT_NE(nullptr, exprLoopOpLhs);
	EXPECT_EQ("i", *exprLoopOpLhs);

	EXPECT_EQ(1, exprLoop->condition.operation.size());
	EXPECT_EQ("<", exprLoop->condition.operation[0].op);

	string* exprLoopOpRhs = boost::get<string>(&exprLoop->condition.operation[0].rhs);
	EXPECT_TRUE(exprLoopOpRhs != nullptr);
	EXPECT_EQ("4", *exprLoopOpRhs);
}

TEST(ASTTest, FuncCallInIfStmt) {
	const auto testProgram =
		"marklar func1(marklar a) {"
		"  return 0;"
		"}"
		"marklar main() {"
		"  if (func1(45) > 0) {"
		"    return 1;"
		"  }"
		"  return 0;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	base_expr* expr = boost::get<base_expr>(&root);
	EXPECT_NE(nullptr, expr);

	func_expr* exprF_main = boost::get<func_expr>(&expr->children[1]);
	EXPECT_NE(nullptr, exprF_main);
	
	if_expr* exprIf = boost::get<if_expr>(&exprF_main->expressions[0]);
	EXPECT_TRUE(exprIf != nullptr);

	call_expr* callExpr = boost::get<call_expr>(&exprIf->condition.lhs);
	EXPECT_NE(nullptr, callExpr);
}
#endif

