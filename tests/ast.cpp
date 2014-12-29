#include <gtest/gtest.h>

#include <parser.h>

#include <map>
#include <string>
#include <tuple>

#include <boost/variant/get.hpp>

using namespace mhc;
using namespace parser;
using namespace std;


// Debugging helper visitor
#include <iostream>
using namespace std;

class ast_helper : public boost::static_visitor<void> {
public:
	ast_helper() {}
	ast_helper(int indentLevel) : mIndentLevel(indentLevel), mIndentString(mIndentLevel, ' ') {}


	void operator()(const parser::base_expr& expr) {
		cout << mIndentString << "AST Base" << endl;
	}
	void operator()(const parser::algebraic_datatype_decl& decl) {
		cout << mIndentString << "AST Algebraic Datatype Decl: " << decl.type_ctor << endl;

		for (auto itr : decl.components) {
			cout << mIndentString << " ADT Component Type: " << itr << endl;
		}
		for (auto itr : decl.deriving_typeclasses) {
			cout << mIndentString << " ADT Deriving Type: " << itr << endl;
		}
	}
	void operator()(const parser::module_decl& decl) {
		cout << mIndentString << "AST Module Decl: " << decl.module_id << endl;

		for (auto itr : decl.body) {
			ast_helper astHelper(mIndentLevel + 1);
			boost::apply_visitor(astHelper, itr);
		}
	}
	void operator()(const parser::type_synonym_decl& decl) {
		cout << mIndentString << "AST Type Synonym Decl" << endl;
	}
	void operator()(const std::string& str) {
		cout << mIndentString << "AST String: " << str << endl;
	}

private:
	int mIndentLevel = 0;
	string mIndentString;
};





TEST(ASTTest, DataType) {
	// Taken from Real World Haskell, Ch. 3
	const auto input =
		"data BookInfo = Book Int String [String]"
		"                deriving (Show, Test)";

	base_expr_node root;
	EXPECT_TRUE(parse(input, root));

	const auto expr = boost::get<base_expr>(&root);
	EXPECT_TRUE(expr != nullptr);
	EXPECT_EQ(1, expr->children.size());

	const auto module = boost::get<module_decl>(&expr->children[0]);
	EXPECT_TRUE(module != nullptr);
	EXPECT_EQ(1, module->body.size());

	const auto adt = boost::get<algebraic_datatype_decl>(&module->body[0]);
	EXPECT_TRUE(adt != nullptr);

	EXPECT_EQ("BookInfo", adt->type_ctor);
	EXPECT_EQ("Book",     adt->components[0]);
	EXPECT_EQ("Int",      adt->components[1]);
	EXPECT_EQ("String",   adt->components[2]);
	EXPECT_EQ("[String]", adt->components[3]);
	EXPECT_EQ("Show",     adt->deriving_typeclasses[0]);
	EXPECT_EQ("Test",     adt->deriving_typeclasses[1]);

	/*
	// Debug
	for (auto e : expr->children) {
		ast_helper astDebug;
		boost::apply_visitor(astDebug, e);
	}
	*/
}

