#pragma once

#include <boost/variant/recursive_variant.hpp>

#include <string>
#include <vector>

namespace parser {

	struct base_expr;
	struct func_expr;
	struct decl_expr;
	struct operator_expr;
	struct call_expr;
	struct return_expr;
	struct if_expr;
	struct binary_op;
	struct while_loop;
	struct var_assign;
	

	typedef boost::variant<
		boost::recursive_wrapper<base_expr>,
		boost::recursive_wrapper<func_expr>,
		boost::recursive_wrapper<decl_expr>,
		boost::recursive_wrapper<operator_expr>,
		boost::recursive_wrapper<call_expr>,
		boost::recursive_wrapper<return_expr>,
		boost::recursive_wrapper<if_expr>,
		boost::recursive_wrapper<binary_op>,
		boost::recursive_wrapper<while_loop>,
		boost::recursive_wrapper<var_assign>,
		std::string
	> base_expr_node;

	struct operation {
		std::string op;
		base_expr_node rhs;
	};

	struct binary_op {
		base_expr_node lhs;
		std::vector<parser::operation> operation;
	};

	struct base_expr {
		std::vector<base_expr_node> children;
	};

	struct return_expr {
		base_expr_node ret;
	};
	
	struct func_expr {
		std::string functionName;
		std::vector<std::string> args;
		std::vector<base_expr_node> declarations;
		std::vector<base_expr_node> expressions;
	};

	struct decl_expr {
		std::string declName;
		base_expr_node val;
	};

	struct operator_expr {
		std::string valLHS;
		std::vector<std::string> op_and_valRHS;
	};

	struct call_expr {
		std::string funcName;
		std::vector<base_expr_node> values;
	};

	struct if_expr {
		binary_op condition;
		std::vector<base_expr_node> thenBranch;
		std::vector<base_expr_node> elseBranch;
	};

	struct while_loop {
		binary_op condition;
		std::vector<base_expr_node> loopBody;
	};

	struct var_assign {
		std::string varName;
		base_expr_node varRhs;
	};

}


namespace marklar {

	bool parse(const std::string& str);

	bool parse(const std::string& str, parser::base_expr_node& root);

}

