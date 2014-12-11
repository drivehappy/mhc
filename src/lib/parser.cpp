#include "parser.h"

#define BOOST_SPIRIT_DEBUG 

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/repository/include/qi_distinct.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>

#include <regex>
#include <string>
#include <vector>


using namespace std;

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;
namespace fusion = boost::fusion;
namespace repo = boost::spirit::repository;


// Old Marklar rules
BOOST_FUSION_ADAPT_STRUCT(
	parser::operation,
	(std::string, op)
	(parser::base_expr_node, rhs)
)

BOOST_FUSION_ADAPT_STRUCT(
	parser::binary_op,
	(parser::base_expr_node, lhs)
	(std::vector<parser::operation>, operation)
)

BOOST_FUSION_ADAPT_STRUCT(
	parser::base_expr,
	(std::vector<parser::base_expr_node>, children)
)

BOOST_FUSION_ADAPT_STRUCT(
	parser::decl_expr,
	(std::string, declName)
	(parser::base_expr_node, val)
)

BOOST_FUSION_ADAPT_STRUCT(
	parser::func_expr,

	(std::string, functionName)
	(std::vector<std::string>, args)
	(std::vector<parser::base_expr_node>, declarations)
	(std::vector<parser::base_expr_node>, expressions)
)

BOOST_FUSION_ADAPT_STRUCT(
	parser::operator_expr,
	(std::string, valLHS)
	(std::vector<std::string>, op_and_valRHS)
)

BOOST_FUSION_ADAPT_STRUCT(
	parser::return_expr,
	(parser::base_expr_node, ret)
)

BOOST_FUSION_ADAPT_STRUCT(
	parser::call_expr,
	(std::string, funcName)
	(std::vector<parser::base_expr_node>, values)
)

BOOST_FUSION_ADAPT_STRUCT(
	parser::if_expr,
	(parser::binary_op, condition)
	(std::vector<parser::base_expr_node>, thenBranch)
	(std::vector<parser::base_expr_node>, elseBranch)
)

BOOST_FUSION_ADAPT_STRUCT(
	parser::while_loop,
	(parser::binary_op, condition)
	(std::vector<parser::base_expr_node>, loopBody)
)

BOOST_FUSION_ADAPT_STRUCT(
	parser::var_assign,
	(std::string, varName)
	(parser::base_expr_node, varRhs)
)


namespace parser {

	qi::char_type char_;

	struct _reserved_symbols : qi::symbols<char, unsigned int>
	{
		_reserved_symbols()
		{
			/*
			reservedid %=
				  qi::string("case") | "class" | "data" | "default" | "deriving" | "do" | "else"
				| "foreign" | "if" | "import" | "in" | "infix" | "infixl"
				| "infixr" | "instance" | "let" | "module" | "newtype" | "of"
				| "then" | "type" | "where" | "_"
			*/
			
			add
			("_",    1)
			("case", 2)
			;
		}
	} reserved_symbols;

	// Skip parser used from: http://www.boost.org/doc/libs/1_56_0/libs/spirit/example/qi/compiler_tutorial/mini_c/skipper.hpp
	template <typename Iterator>
    struct skipper : qi::grammar<Iterator>
    {
        skipper() : skipper::base_type(start)
        {
            start =
                   qi::blank
				|  ncomment
				|  comment
                ;

			comment =
				   qi::lit("--")
				>> *(char_ - (qi::eol || qi::eoi))
				>> (qi::eol || qi::eoi);

			ncomment =
				   qi::lit("{-")
				>> *(char_ - (qi::lit("-}") || "{-"))
				>> -(ncomment)
				>> "-}"
				;

			#if 0
			// Debugging
			BOOST_SPIRIT_DEBUG_NODE(start);
			BOOST_SPIRIT_DEBUG_NODE(ncomment);
			#endif
        }

        qi::rule<Iterator> start;
        qi::rule<Iterator> ncomment;
        qi::rule<Iterator> comment;
    };

	template <typename Iterator>
	struct mhc_grammar : qi::grammar<Iterator, base_expr_node(), skipper<Iterator>>
	{
		mhc_grammar() : mhc_grammar::base_type(start)
		{
			start %= rootNode;

			rootNode %=
				   qi::eps
				//>> *reservedid
				>> *varid
				>> qi::eoi;

			varid %=
				   (!reservedid)
				>> (qi::char_("a-z_") >> *qi::char_("a-z_A-Z0-9'"));

			reservedid %=
				   reserved_symbols
				>> !(char_("a-zA-Z0-9'_"));

			/*
			reservedid %=
				  qi::string("case") | "class" | "data" | "default" | "deriving" | "do" | "else"
				| "foreign" | "if" | "import" | "in" | "infix" | "infixl"
				| "infixr" | "instance" | "let" | "module" | "newtype" | "of"
				| "then" | "type" | "where" | "_"
				;
			*/

			funcExpr %=
				  "mhc"
				>> varName
				>> '(' >> *(varDef % ',') >> ')'
				>> '{'
				>> *varDecl
				>> *baseExpr
				>> -returnExpr
				>> '}'
				;

			varDef %=
				  "mhc"
				>> varName
				;

			varDecl %=
				   varDef
				>> -('=' >> (op_expr | value))
				>> ';'
				;

			op_expr %=
				   factor
				>> *(op >> factor);

			factor %=
				  qi::lit('(') >> op_expr >> ')'
				| callExpr
				| value;

			baseExpr %= intLiteral | returnExpr | (callExpr >> ';') | ifExpr | varDecl | varAssign | whileLoop;

			// Small hack to only allow op_expr, but allow boost::fusion to use
			// the base_node_expr type still (if we didn't, then baseExpr would
			// be used, and it would parse odd things)
			callBaseExpr %= op_expr;

			callExpr %=
				   varName
				>> '(' >> *(callBaseExpr % ',') >> ')'
				;

			returnExpr %=
				   "return"
				>> (callExpr | op_expr | value)
				>> ';'
				;

			ifExpr %=
				   qi::lit("if")
				>> '('
				>> op_expr
				>> ')' >> '{'
				>> *baseExpr
				>> '}'
				>> -(qi::lit("else") >> '{' >> *baseExpr >> '}')
				;

			whileLoop %=
				   qi::lit("while")
				>> '('
				>> op_expr
				>> ')' >> '{'
				>> *baseExpr
				>> '}'
				;

			varAssign %=
				   varName
				>> ('=' >> (op_expr | value))
				>> ';'
				;

			varName %= qi::char_("a-zA-Z_") >> *qi::char_("a-zA-Z_0-9");
			intLiteral %= +qi::char_("0-9");
			value %= (varName | intLiteral);

			// '>>' before the next '>' or else it will be matched as greater-than
			op %=
				  ascii::string(">>")
				| ascii::string("<<")
				| ascii::string(">=")
				| ascii::string("<=")
				| ascii::string("!=")
				| ascii::string("==")
				| ascii::string("||")
				| ascii::string("&&")
				| qi::char_("+<>%/*&")
				| qi::char_("\\-")
				;

			// Debugging
			/*
			BOOST_SPIRIT_DEBUG_NODE(varid);
			BOOST_SPIRIT_DEBUG_NODE(reservedid);
			BOOST_SPIRIT_DEBUG_NODE(funcExpr);
			BOOST_SPIRIT_DEBUG_NODE(varDecl);
			BOOST_SPIRIT_DEBUG_NODE(baseExpr);
			BOOST_SPIRIT_DEBUG_NODE(ifExpr);
			BOOST_SPIRIT_DEBUG_NODE(op_expr);
			BOOST_SPIRIT_DEBUG_NODE(returnExpr);
			BOOST_SPIRIT_DEBUG_NODE(callExpr);
			BOOST_SPIRIT_DEBUG_NODE(op);
			BOOST_SPIRIT_DEBUG_NODE(value);
			BOOST_SPIRIT_DEBUG_NODE(intLiteral);
			BOOST_SPIRIT_DEBUG_NODE(factor);
			BOOST_SPIRIT_DEBUG_NODE(whileLoop);
			*/
		}

		qi::rule<Iterator, base_expr_node(),	skipper<Iterator>> start;
		qi::rule<Iterator, base_expr(),			skipper<Iterator>> rootNode;
		qi::rule<Iterator, string(),			skipper<Iterator>> varid;
		qi::rule<Iterator, string(),			skipper<Iterator>> reservedid;


		// Old Marklar eventually delete these
		qi::rule<Iterator, func_expr(), skipper<Iterator>> funcExpr;
		qi::rule<Iterator, decl_expr(), skipper<Iterator>> varDecl;
		qi::rule<Iterator, string(), skipper<Iterator>> varDef;
		qi::rule<Iterator, binary_op(), skipper<Iterator>> op_expr;
		qi::rule<Iterator, base_expr_node(), skipper<Iterator>> baseExpr;
		qi::rule<Iterator, base_expr_node(), skipper<Iterator>> callBaseExpr;
		qi::rule<Iterator, return_expr(), skipper<Iterator>> returnExpr;
		qi::rule<Iterator, call_expr(), skipper<Iterator>> callExpr;
		qi::rule<Iterator, if_expr(), skipper<Iterator>> ifExpr;
		qi::rule<Iterator, while_loop(), skipper<Iterator>> whileLoop;
		qi::rule<Iterator, var_assign(), skipper<Iterator>> varAssign;

		qi::rule<Iterator, base_expr_node(), skipper<Iterator>> factor;
		qi::rule<Iterator, std::string(), skipper<Iterator>> varName;
		qi::rule<Iterator, std::string(), skipper<Iterator>> intLiteral;
		qi::rule<Iterator, std::string(), skipper<Iterator>> value;
		qi::rule<Iterator, std::string(), skipper<Iterator>> op;
	};

}

namespace mhc {

	bool parse(const std::string& str, parser::base_expr_node& root) {
		parser::mhc_grammar<std::string::const_iterator> p;
		parser::skipper<std::string::const_iterator> s;
		const bool r = qi::phrase_parse(str.begin(), str.end(), p, s, root);

		return r;
	}

	bool parse(const std::string& str) {
		parser::base_expr_node root;

		return parse(str, root);
	}

}

