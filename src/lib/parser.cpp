#include "parser.h"

#define BOOST_SPIRIT_DEBUG 

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_lexeme.hpp>
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

	// Symbols for reservedid
	struct _reservedid_symbols : qi::symbols<char, unsigned int>
	{
		_reservedid_symbols()
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
	} reservedid_symbols;

	// Symbols for reservedop
	struct _reservedop_symbols : qi::symbols<char, unsigned int>
	{
		_reservedop_symbols()
		{
			add
			("..",  1)
			(":",   2)
			("::",  3)
			("=",   4)
			("\\",  5)
			("|",   6)
			("<-",  7)
			("->",  8)
			("@",   9)
			("~",  10)
			("=>", 11)
			;
		}
	} reservedop_symbols;

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

			// Debugging
			#if 0
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
			start %= program;

			// Haskell Report Chapter2: This is the lexical structure,
			// the Expressions and Declarations/Binds are below (Chapter3 and 4)
			/*
			program %=
				   qi::eps
				>> *lexeme_
				>> qi::eoi
				;
			*/

			lexeme_ %=
				  qvarid
				| qconid
				| qvarsym
				| qconsym
				| special
				| reservedop
				| reservedid
				| literal
				;

			literal %=
				  integer
				| float_ 
				| char__
				| string__
				;

			special %= qi::char_("(),;[]`{}");

			ascSymbol %= qi::char_("!#$%&*+./<=>?@\\^|-~:");

			symbol %=
				   (!special)
				//>> (qi::char_("A-Z"));
				>> ascSymbol;

			graphic %=
				  qi::char_("a-z")
				| qi::char_("A-Z")
				| symbol
				| qi::digit
				| special
				| '"'
				| '\''
				;

			// Identifiers
			varid %=
				   (!reservedid)
				>> qi::lexeme[(qi::char_("a-z_") >> *qi::char_("a-z_A-Z0-9'"))];

			conid %=
				   qi::char_("A-Z") >> *qi::char_("a-zA-Z0-9'");

			reservedid %=
				   reservedid_symbols
				>> !(char_("a-zA-Z0-9'_"));

			// Operators
			varsym %=
				   (!reservedop)
				>> (
				       (!qi::char_(":"))
				    >> (symbol >> *symbol)
				   );

			reservedop %= reservedop_symbols;

			// Type variable
			tyvar %= varid;

			// Type contructor
			tycon %= conid;

			// Type class
			tycls %= conid;

			// Modules
			//modid %= (qi::lexeme[*(conid >> '.')]) >> conid;
			conid_noskip %= qi::char_("A-Z") >> *qi::char_("a-zA-Z0-9'");
			modid %=
				conid |
				(
					  *(qi::lexeme[(conid_noskip >> '.')])
				   >> conid_noskip
				)
				;

			// Qualified
			qvarid %= -(modid >> '.') >> varid;
			qconid %= -(modid >> '.') >> conid;
			qtycon %= -(modid >> '.') >> tycon;
			qtycls %= -(modid >> '.') >> tycls;
			qvarsym %= -(modid >> '.') >> varsym;
			qconsym %= -(modid >> '.') >> consym;

			// Numeric Literals
			octit %= qi::char_("0-7");
			hexit %= qi::digit | qi::char_("A-F") | qi::char_("a-f");

			decimal %= qi::lexeme[+qi::digit];
			octal %= +octit;
			hexadecimal %= +hexit;

			integer %=
				  qi::lexeme[(("0o" >> octal) | ("0O" >> octal))]
				| (qi::lexeme[("0x" >> hexadecimal)] | qi::lexeme[("0X" >> hexadecimal)])
				| decimal
				;

			float_ %=
				  (decimal >> "." >> decimal >> -(exponent))
				| (decimal >> exponent)
				;

			exponent %=
				   (qi::lit('e') | 'E')
				>> -(qi::lit('+') | '-')
				>> decimal
				;

			// Character and String Literals
			char__ %=
				   "'"
				>> ( (graphic - (qi::lit("'") | "\\"))
				   | escape
				   )
				>> "'"
				;
 
			string__ %=
				   '"'
				>> -(graphic)
				;

			escape %=
				   '\\'
				>> ( qi::char_("abfnrtv\\\"'&")
				   //| // TODO: ascii
				     | decimal
				     | ('o' >> octal)
				     | ('x' >> hexadecimal)
				   )
				;
	

			// Haskell Report Chapter3: Expressions
			var %= varid; /* TODO: | varsym*/

			

			// Haskell Report Chapter4: Declarations and Bindings
			program %=
				   qi::eps
				>> module
				>> qi::eoi
				;

			module %=
				  ("module" >> modid >> /* TODO: -(exports) >> */ "where" >> body)
				| body;

			body %=
				  /* TODO: '{' >> impdecls >> ';' >> topdecls >> '}' >>*/  // ch5: Modules
				  /*'{' >> topdecls >> '}';*/
				  *topdecls;

			topdecls %=
				  topdecl % ';';

			topdecl %=
				  ("type" >> simpletype >> "=" >> type)
				| ("class" >> /* TODO: -(scontext >> "=>") >>*/ tycls >> tyvar >> -("where" >> cdecls))
				| decl;

			decls %=
				  ("{" >> (decl % ';') >> "}");

			decl %=
				  gendecl;
				/* TODO: | (funlhs | var) rhs*/

			cdecls %=
				  ("{" >> (cdecl % ';') >> "}");

			cdecl %=
				  gendecl;
				/* TODO: | (funlhs | var) rhs*/

			gendecl %=
				  (vars >> "::" >> /* TODO: -(context >> "=>") >>"*/ type)
				| (fixity >> -integer >> ops);
				/* TODO: Empty decl, unsure how to handle this */

			varop %=
				  varsym
				| ("`" >> varid  >> "`");

			conop %=
				  consym
				| ("`" >> conid  >> "`");

			op %= varop | conop;

			ops %= (op % ',');
			vars %= (var % ',');
			fixity %= qi::lit("infixl") | "infixr" | "infix";

			// CH 4.1.2: Syntax of Types
			type %= btype >> -("->" >> type);

			btype %= -(btype) >> atype;

			atype %=
				  gtycon
				| tyvar
				| ("(" >> (type >> ',' >> type >> ',' >> (type % ',')) >> ")")
				| ("[" >> type >> "]")
				| ("(" >> type >> ")");

			gtycon %=
				  qtycon
				| "()"									// unit type
				| "[]"									// list constructor
				| "(->)"								// function constructor
				| ("(," >> (*qi::char_(',')) >> ")");	// tupling constructor

			// CH 4.2.1: Algebraic Datatype Decls
			simpletype %=
				  tycon >> *tyvar;

			// Debugging
			/*
			BOOST_SPIRIT_DEBUG_NODE(start);
			BOOST_SPIRIT_DEBUG_NODE(program);
			BOOST_SPIRIT_DEBUG_NODE(module);
			BOOST_SPIRIT_DEBUG_NODE(modid);
			BOOST_SPIRIT_DEBUG_NODE(body);
			BOOST_SPIRIT_DEBUG_NODE(lexeme_);
			BOOST_SPIRIT_DEBUG_NODE(literal);
			BOOST_SPIRIT_DEBUG_NODE(varid);
			BOOST_SPIRIT_DEBUG_NODE(conid);
			*/
			#if 0
			BOOST_SPIRIT_DEBUG_NODE(reservedid);
			BOOST_SPIRIT_DEBUG_NODE(special);
			BOOST_SPIRIT_DEBUG_NODE(symbol);
			BOOST_SPIRIT_DEBUG_NODE(reservedop);
			BOOST_SPIRIT_DEBUG_NODE(conid_noskip);
			BOOST_SPIRIT_DEBUG_NODE(modid);
			BOOST_SPIRIT_DEBUG_NODE(qvarid);
			BOOST_SPIRIT_DEBUG_NODE(qconid);
			#endif

			#if 0
			BOOST_SPIRIT_DEBUG_NODE(integer);
			BOOST_SPIRIT_DEBUG_NODE(float_);
			BOOST_SPIRIT_DEBUG_NODE(exponent);
			BOOST_SPIRIT_DEBUG_NODE(decimal);
			BOOST_SPIRIT_DEBUG_NODE(octal);
			BOOST_SPIRIT_DEBUG_NODE(hexadecimal);
			BOOST_SPIRIT_DEBUG_NODE(hexit);

			BOOST_SPIRIT_DEBUG_NODE(graphic);
			BOOST_SPIRIT_DEBUG_NODE(char__);
			BOOST_SPIRIT_DEBUG_NODE(string__);
			#endif
		}

		qi::rule<Iterator, base_expr_node(),	skipper<Iterator>> start;
		qi::rule<Iterator, base_expr(),			skipper<Iterator>> program;
		qi::rule<Iterator, string(),			skipper<Iterator>> lexeme_;
		qi::rule<Iterator, string(),			skipper<Iterator>> literal;

		qi::rule<Iterator, string(),			skipper<Iterator>> varid;
		qi::rule<Iterator, string(),			skipper<Iterator>> conid;
		qi::rule<Iterator, string(),			skipper<Iterator>> reservedid;

		qi::rule<Iterator, string(),			skipper<Iterator>> ascSymbol;
		qi::rule<Iterator, string(),			skipper<Iterator>> special;
		qi::rule<Iterator, string(),			skipper<Iterator>> symbol;
		qi::rule<Iterator, string(),			skipper<Iterator>> graphic;
		qi::rule<Iterator, string(),			skipper<Iterator>> varsym;
		qi::rule<Iterator, string(),			skipper<Iterator>> consym;
		qi::rule<Iterator, string(),			skipper<Iterator>> reservedop;

		qi::rule<Iterator, string(),			skipper<Iterator>> tyvar;
		qi::rule<Iterator, string(),			skipper<Iterator>> tycon;
		qi::rule<Iterator, string(),			skipper<Iterator>> tycls;
		qi::rule<Iterator, string(),			skipper<Iterator>> modid;
		qi::rule<Iterator, string()								 > conid_noskip;

		qi::rule<Iterator, string(),			skipper<Iterator>> qvarid;
		qi::rule<Iterator, string(),			skipper<Iterator>> qconid;
		qi::rule<Iterator, string(),			skipper<Iterator>> qtycon;
		qi::rule<Iterator, string(),			skipper<Iterator>> qtycls;
		qi::rule<Iterator, string(),			skipper<Iterator>> qvarsym;
		qi::rule<Iterator, string(),			skipper<Iterator>> qconsym;
		
		qi::rule<Iterator, string()								 > octit;
		qi::rule<Iterator, string()								 > hexit;
		qi::rule<Iterator, string(),			skipper<Iterator>> decimal;
		qi::rule<Iterator, string()								 > octal;
		qi::rule<Iterator, string()								 > hexadecimal;

		qi::rule<Iterator, string(),			skipper<Iterator>> integer;
		qi::rule<Iterator, string(),			skipper<Iterator>> float_;
		qi::rule<Iterator, string(),			skipper<Iterator>> exponent;

		qi::rule<Iterator, string(),			skipper<Iterator>> char__;
		qi::rule<Iterator, string(),			skipper<Iterator>> string__;
		qi::rule<Iterator, string(),			skipper<Iterator>> escape;

		qi::rule<Iterator, string(),			skipper<Iterator>> module;
		qi::rule<Iterator, string(),			skipper<Iterator>> body;
		qi::rule<Iterator, string(),			skipper<Iterator>> topdecls;
		qi::rule<Iterator, string(),			skipper<Iterator>> topdecl;
		qi::rule<Iterator, string(),			skipper<Iterator>> decls;
		qi::rule<Iterator, string(),			skipper<Iterator>> decl;
		qi::rule<Iterator, string(),			skipper<Iterator>> cdecls;
		qi::rule<Iterator, string(),			skipper<Iterator>> cdecl;
		qi::rule<Iterator, string(),			skipper<Iterator>> gendecl;

		qi::rule<Iterator, string(),			skipper<Iterator>> ops;
		qi::rule<Iterator, string(),			skipper<Iterator>> vars;
		qi::rule<Iterator, string(),			skipper<Iterator>> fixity;

		// CH 3: Expressions
		qi::rule<Iterator, string(),			skipper<Iterator>> var;
		qi::rule<Iterator, string(),			skipper<Iterator>> varop;
		qi::rule<Iterator, string(),			skipper<Iterator>> conop;
		qi::rule<Iterator, string(),			skipper<Iterator>> op;

		// CH 4.1.2 Syntax of Types
		qi::rule<Iterator, string(),			skipper<Iterator>> type;
		qi::rule<Iterator, string(),			skipper<Iterator>> btype;
		qi::rule<Iterator, string(),			skipper<Iterator>> atype;
		qi::rule<Iterator, string(),			skipper<Iterator>> gtycon;

		// CH 4.2.1 Algebraic Datatype
		qi::rule<Iterator, string(),			skipper<Iterator>> simpletype;
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

