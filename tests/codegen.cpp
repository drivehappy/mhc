#include <gtest/gtest.h>

#include <parser.h>
#include <codegen.h>

#include <memory>
#include <string>

#include <boost/variant/get.hpp>

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/IRBuilder.h>
#include "llvm/IR/LLVMContext.h"
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace mhc;
using namespace parser;

using namespace llvm;
using namespace std;


namespace {

	unique_ptr<Module> codegenTest(const base_expr_node& root) {
		LLVMContext &context = getGlobalContext();
		unique_ptr<Module> module(new Module("", context));
		IRBuilder<> builder(getGlobalContext());

		ast_codegen codeGenerator(module.get(), builder);

		// Codegen for each expression we've found in the root AST
		const base_expr* expr = boost::get<base_expr>(&root);
		for (auto& itr : expr->children) {
			boost::apply_visitor(codeGenerator, itr);
		}

		return module;
	}

}

TEST(CodegenTest, BasicFunction) {
	const auto testProgram =
		"marklar main() {"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	// Expect this to fail since we require a 'return'
	auto module = codegenTest(root);
	EXPECT_TRUE(verifyModule(*module, &errorOut)) << errorInfo;
}
 
TEST(CodegenTest, FunctionSingleDecl) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 0;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	// Expect this to fail since we require a 'return'
	auto module = codegenTest(root);
	EXPECT_TRUE(verifyModule(*module, &errorOut)) << errorInfo;
}
 
TEST(CodegenTest, FunctionSingleDeclReturn) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 0;"
		"  return i;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, FunctionMultiDeclAssign) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 1 + 2;"
		"  marklar j = i + 2;"
		"  marklar k = i + j;"
		"  return k;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, FunctionMultiDeclSum) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 2;"
		"  marklar j = 5;"
		"  return i + j;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, MultipleFunction) {
	const auto testProgram =
		"marklar bar() {"
		"  marklar a = 0;"
		"  marklar b = 2;"
		"  return a + b + 0;"
		"}"
		"marklar foo() {"
		"  marklar a = 4;"
		"  return a + 0;"
		"}"
		"marklar main() {"
		"  return 0 + 1;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, FunctionArgs) {
	const auto testProgram =
		"marklar main(marklar a, marklar b) {"
		"  return 1;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, FunctionMultipleArgs) {
	const auto testProgram =
		"marklar bar(marklar a) {"
		"  marklar b = 2;"
		"  return 1 + b + 0;"
		"}"
		"marklar main(marklar a, marklar b) {"
		"  return 1;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, FunctionUseArgs) {
	const auto testProgram =
		"marklar main(marklar a) {"
		"  return a;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, FunctionCall) {
	const auto testProgram =
		"marklar foo(marklar a) {"
		"  return a + 1;"
		"}"
		"marklar main(marklar a) {"
		"  return foo(a);"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, IfElseStmt) {
	const auto testProgram =
		"marklar main() {"
		"  if (3 > 4) {"
		"    return 1;"
		"  } else {"
		"    return 2;"
		"  }"
		"  return 3;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	auto module = codegenTest(root);

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	module->print(errorOut, nullptr);
	if (verifyModule(*module, &errorOut)) {
		cerr << "Error: " << errorInfo << endl;
	}

	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, OperatorGreaterThan) {
	const auto testProgram =
		"marklar main() {"
		"  if (3 > 4) {"
		"    return 1;"
		"  }"
		"  return 2;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, OperatorEqual) {
	const auto testProgram =
		"marklar main() {"
		"  if (4 == 4) {"
		"    return 1;"
		"  }"
		"  return 2;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, OperatorModulo) {
	const auto testProgram =
		"marklar main() {"
		"  marklar a = 5 % 3;"
		"  if (a == 2) {"
		"    return 1;"
		"  }"
		"  return 0;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, Assignment) {
	const auto testProgram =
		"marklar main() {"
		"  marklar a = 3;"
		"  a = a + 1;"
		"  return a;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, WhileStmt) {
	const auto testProgram =
		"marklar main() {"
		"  marklar a = 2;"
		"  marklar b = 6;"
		"  while (a < b) {"
		"    a = a + 1;"
		"  }"
		"  return a;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, LogicalOR) {
	const auto testProgram =
		"marklar main() {"
		"  marklar a = 0;"
		"  marklar b = 0;"
		"  if ((a == 0) || (b == 0)) {"
		"    return 2;"
		"  }"
		"  return 1;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, SameVariableNameDiffFunctions) {
	const auto testProgram =
		"marklar fib(marklar a) {"
		"  return a;"
		"}"
		"marklar main() {"
		"  marklar a = 10;"
		"  return fib(a);"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, FuncCallInIfStmt) {
	const auto testProgram =
		"marklar func1(marklar a) {"
		"	return 0;"
		"}"
		"marklar main() {"
		"	if (func1(1) > 0) {"
		"		return 1;"
		"	}"
		"   return 0;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, WhileWithReturnStmt) {
	const auto testProgram =
		"marklar main() {"
		"	while (1 == 1) {"
		"		return 1;"
		"	}"
		"   return 0;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}

TEST(CodegenTest, FuncWithEarlyReturnStmt) {
	const auto testProgram =
		"marklar main() {"
		"   return 2;"
		"	while (1 == 1) {"
		"		return 1;"
		"	}"
		"   return 0;"
		"}";

	base_expr_node root;
	EXPECT_TRUE(parse(testProgram, root));

	string errorInfo;
	raw_string_ostream errorOut(errorInfo);

	auto module = codegenTest(root);
	EXPECT_FALSE(verifyModule(*module, &errorOut)) << errorInfo;
}






