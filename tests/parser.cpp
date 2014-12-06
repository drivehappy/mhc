#include <gtest/gtest.h>

#include <parser.h>

using namespace marklar;


TEST(ParserTest, BasicFunction) {
	const auto testProgram =
		"marklar main() {"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, BasicComment) {
	const auto testProgram =
		"/* This is a comment */"
		"marklar main() {"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, InvalidComment) {
	const auto testProgram =
		"/* This is a comment without the required * /"
		"marklar main() {"
		"}";

	EXPECT_FALSE(parse(testProgram));
}

TEST(ParserTest, FunctionSingleDecl) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 0;"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, FunctionMultiDecl) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 0;"
		"  marklar j = 0;"
		"  marklar k = 0;"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, FunctionDeclAssign) {
	const auto testProgram =
		"marklar main() {"
		"  marklar r = 1 + 2;"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, FunctionMultiDeclAssign) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 1 + 2;"
		"  marklar j = i + 2;"
		"  marklar k = i + j;"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, FunctionReturn) {
	const auto testProgram =
		"marklar main() {"
		"  return 1;"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, FunctionReturnComplex) {
	const auto testProgram =
		"marklar main() {"
		"  return a + b + c + 0 + 1 + d;"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, MultipleFunction) {
	const auto testProgram =
		"marklar foo() {}"
		"marklar main() {}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, MultipleComplexFunction) {
	const auto testProgram =
		"marklar bar() {"
		"  marklar a = 0;"
		"  marklar b = 2;"
		"  return a + b + 0;"
		"}"
		"marklar foo() {"
		"  return a + 0;"
		"}"
		"marklar main() {"
		"  return 0 + 1;"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, FunctionArgs) {
	const auto testProgram =
		"marklar main(marklar a, marklar b) {"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, FunctionCall) {
	const auto testProgram =
		"marklar foo() {}"
		"marklar main() {"
		"  foo();"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, FunctionCallArgs) {
	const auto testProgram =
		"marklar foo(marklar a) {}"
		"marklar main() {"
		"  foo(45);"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, FunctionCallArgsComplex) {
	const auto testProgram =
		"marklar bar(marklar a, marklar b) {}"
		"marklar foo(marklar a) {"
		"  return bar(a, 5);"
		"}"
		"marklar main() {"
		"  return foo(45);"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, FunctionIfStmt) {
	const auto testProgram =
		"marklar main() {"
		"  if (i < 4) {"
		"  }"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, FunctionIfElseStmt) {
	const auto testProgram =
		"marklar main() {"
		"  if (i < 4) {"
		"  } else {"
		"  }"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, Assignment) {
	const auto testProgram =
		"marklar main() {"
		"  marklar a = 0;"
		"  a = a + 1;"
		"  return a;"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, WhileStmt) {
	const auto testProgram =
		"marklar main() {"
		"  while (i < 4) {"
		"    if (i > 5) {"
		"    } else {"
		"    }"
		"  }"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, LogicalOR) {
	const auto testProgram =
		"marklar main() {"
		"  marklar a = 0;"
		"  marklar b = 0;"
		"  if (a || b) {"
		"    return 2;"
		"  }"
		"  return 1;"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, Division) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 5 / 3;"
		"  return i;"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, Subtraction) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 5 - 3;"
		"  return i;"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, RightShift) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 257 >> 8;"
		"  return i;"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, Multiplication) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 8 * 4;"
		"  return i;"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, ComplexEulerProblem1) {
	const auto testProgram =
		"marklar main() {"
		"  marklar sum = 0;"
		"  marklar i = 0;"
		"  while (i < 1000) {"
		"    if (((i % 5) == 0) || ((i % 3) == 0)) {"
		"      sum = sum + i;"
		"    }"
		"    i = i + 1;"
		"  }"
		"  return sum;"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, FuncCallInIfStmt) {
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

	EXPECT_TRUE(parse(testProgram));
}

TEST(ParserTest, LogicalAnd) {
	const auto testProgram =
		"marklar main() {"
		"  while ((0 != 1) && (0 != 1)) {"
		"    return 1;"
		"  }"
		"  return 0;"
		"}";

	EXPECT_TRUE(parse(testProgram));
}

