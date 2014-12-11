#include <gtest/gtest.h>

#include <sys/wait.h>

#include <algorithm>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/scope_exit.hpp>

#include <driver.h>


using namespace mhc;
using namespace std;

#if 0
// Unit test helpers
namespace {

	const string g_outputBitCode = "output.bc";
	const string g_outputExe = "a.out";

	vector<string> g_cleanupFiles = {
		g_outputExe,
		g_outputBitCode,
		"output_opt.bc",
		"output.o",
	};

	void cleanupFiles() {
		for (auto& itr : g_cleanupFiles) {
			boost::filesystem::remove(itr);
		}
	}

	bool createExe(const string& testProgram) {
		if (!driver::generateOutput(testProgram, g_outputBitCode)) {
			return false;
		}
		if (!driver::optimizeAndLink(g_outputBitCode, g_outputExe)) {
			return false;
		}

		return true;
	}

	int runExecutable(const string& exe) {
		const int r = system(("./" + exe).c_str());
		if (r == -1) {
			return -1;
		}

		return WEXITSTATUS(r);
	}

}

TEST(DriverTest, BasicFunction) {
	const auto testProgram =
		"marklar main() {"
		"  return 3;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(3, runExecutable(g_outputExe));
}

TEST(DriverTest, FunctionSingleDecl) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 2;"
		"  return i;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(2, runExecutable(g_outputExe));
}

TEST(DriverTest, FunctionMultiDecl) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 2;"
		"  marklar j = 5;"
		"  return j;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(5, runExecutable(g_outputExe));
}

TEST(DriverTest, FunctionMultiDeclSum) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 2;"
		"  marklar j = 5;"
		"  return i + j;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(7, runExecutable(g_outputExe));
}

TEST(DriverTest, FunctionMultiDeclSumComplex) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 2;"
		"  marklar j = 5;"
		"  return i + j + 6;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(13, runExecutable(g_outputExe));
}

// Tests scoping rules of multiple functions with identical variable names
TEST(DriverTest, MultipleFunction) {
	const auto testProgram =
		"marklar bar() {"
		"  marklar a = 5;"
		"  return a;"
		"}"
		"marklar foo() {"
		"  marklar a = 4;"
		"  return a;"
		"}"
		"marklar main() {"
		"  marklar a = 3;"
		"  return a;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(3, runExecutable(g_outputExe));
}

TEST(DriverTest, FunctionUseArgs) {
	const auto testProgram =
		"marklar main(marklar a) {"
		"  return a;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(1, runExecutable(g_outputExe));
	EXPECT_EQ(2, runExecutable(g_outputExe + " arg1"));
	EXPECT_EQ(3, runExecutable(g_outputExe + " arg1 arg2"));
}

TEST(DriverTest, FunctionCall) {
	const auto testProgram =
		"marklar foo(marklar a) {"
		"  return a + 1;"
		"}"
		"marklar main(marklar a) {"
		"  return foo(a);"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(2, runExecutable(g_outputExe));
	EXPECT_EQ(3, runExecutable(g_outputExe + " arg1"));
	EXPECT_EQ(4, runExecutable(g_outputExe + " arg1 arg2"));
}

TEST(DriverTest, FunctionIfStmtReturnSimple) {
	const auto testProgram =
		"marklar main() {"
		"  if (3 < 4) {"
		"    return 1;"
		"  }"
		"  return 0;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(1, runExecutable(g_outputExe));
}

TEST(DriverTest, FunctionIfStmtReturn) {
	const auto testProgram =
		"marklar main() {"
		"  marklar a = 3;"
		"  marklar b = 4;"
		"  if (a < b) {"
		"    return 1;"
		"  }"
		"  return 0;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(1, runExecutable(g_outputExe));
}

TEST(DriverTest, FunctionIfElseStmtReturn) {
	const auto testProgram =
		"marklar main() {"
		"  marklar a = 3;"
		"  marklar b = 4;"
		"  if (a > b) {"
		"    return 1;"
		"  } else {"
		"    return 0;"
		"  }"
		"  return 2;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(0, runExecutable(g_outputExe));
}

TEST(DriverTest, OperatorLessThan) {
	const auto testProgram =
		"marklar main() {"
		"  if (3 < 4) {"
		"    return 1;"
		"  }"
		"  return 0;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(1, runExecutable(g_outputExe));
}

TEST(DriverTest, OperatorGreaterThan) {
	const auto testProgram =
		"marklar main() {"
		"  if (3 > 4) {"
		"    return 1;"
		"  }"
		"  return 2;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(2, runExecutable(g_outputExe));
}

TEST(DriverTest, OperatorEqual) {
	const auto testProgram =
		"marklar main() {"
		"  if (4 == 4) {"
		"    return 1;"
		"  }"
		"  return 2;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(1, runExecutable(g_outputExe));
}

TEST(DriverTest, OperatorModulo) {
	const auto testProgram =
		"marklar main() {"
		"  marklar a = 5 % 3;"
		"  if (a == 2) {"
		"    return 1;"
		"  }"
		"  return 0;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(1, runExecutable(g_outputExe));
}

TEST(DriverTest, WhileStmt) {
	const auto testProgram =
		"marklar main() {"
		"  marklar a = 2;"
		"  marklar b = 6;"
		"  while (a < b) {"
		"    a = a + 1;"
		"  }"
		"  return a;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(6, runExecutable(g_outputExe));
}

TEST(DriverTest, LogicalOR) {
	const auto testProgram =
		"marklar main() {"
		"  marklar a = 0;"
		"  marklar b = 4;"
		"  if ((a == 0) || (b == 0)) {"
		"    return 2;"
		"  }"
		"  return 1;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(2, runExecutable(g_outputExe));
}

TEST(DriverTest, LogicalAND) {
	const auto testProgram =
		"marklar main() {"
		"  marklar a = 0;"
		"  marklar b = 4;"
		"  if ((a == 0) && (b == 0)) {"
		"    return 2;"
		"  }"
		"  return 1;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(1, runExecutable(g_outputExe));
}

TEST(DriverTest, Division) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 5 / 3;"
		"  return i;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(1, runExecutable(g_outputExe));
}

TEST(DriverTest, Subtraction) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 5 - 3;"
		"  return i;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(2, runExecutable(g_outputExe));
}

TEST(DriverTest, Multiplication) {
	const auto testProgram =
		"marklar main() {"
		"  marklar i = 5 * 3;"
		"  return i;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(15, runExecutable(g_outputExe));
}

TEST(DriverTest, MultiMethods) {
	const auto testProgram =
		"marklar a() {"
		"  return 1;"
		"}"
		"marklar main() {"
		"  marklar i = 5 * 3;"
		"  return i;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(15, runExecutable(g_outputExe));
}

TEST(DriverTest, FuncCallInIfStmt) {
	const auto testProgram =
		"marklar func1(marklar a) {"
		"	return a + 5;"
		"}"
		"marklar main() {"
		"	if (func1(10) < 15) {"
		"		return 1;"
		"	}"
		"   return func1(10);"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(15, runExecutable(g_outputExe));
}

TEST(DriverTest, IfWith2ReturnStmt) {
	const auto testProgram =
		"marklar main() {"
		"  if (1 == 1) {"
		"    return 1;"
		"    return 2;"    
		"  } else {"
		"    return 0;"
		"    return 5;"
		"  }"
		"  return 0;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(1, runExecutable(g_outputExe));
}

TEST(DriverTest, WhileWithReturnStmt) {
	const auto testProgram =
		"marklar main() {"
		"	while (1 == 1) {"
		"		return 1;"
		"	}"
		"   return 0;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(1, runExecutable(g_outputExe));
}

TEST(DriverTest, FuncWithEarlyReturnStmt) {
	const auto testProgram =
		"marklar main() {"
		"	return 2;"
		"	while (1 == 1) {"
		"		return 1;"
		"	}"
		"   return 0;"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(2, runExecutable(g_outputExe));
}

TEST(DriverTest, MultipleChainedFunctionCall) {
	const auto testProgram =
		"marklar unaryFunc(marklar n) {"
		"  return n + 1;"
		"}"
		"marklar binaryFunc(marklar a, marklar b) {"
		"  return unaryFunc((a * a) + (b * b));"
		"}"
		"marklar main() {"
		"  return binaryFunc(1, 5);"
		"}";

	// Cleanup generated intermediate and executable files
	BOOST_SCOPE_EXIT(void) {
		cleanupFiles();
	} BOOST_SCOPE_EXIT_END

	EXPECT_TRUE(createExe(testProgram));

	EXPECT_EQ(27, runExecutable(g_outputExe));
}
#endif
