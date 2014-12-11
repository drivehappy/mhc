#pragma once

#include <map>
#include <string>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>

#include "parser.h"


namespace mhc {

	class ast_codegen : public boost::static_visitor<llvm::Value*> {
	public:
		using symbolType_t = std::map<std::string, llvm::Value*>;

		ast_codegen(llvm::Module* m, llvm::IRBuilder<>& b)
		: m_module(m), m_builder(b) {}

		ast_codegen(const ast_codegen& rhs)
		: m_module(rhs.m_module), m_builder(rhs.m_builder), m_symbolTable(rhs.m_symbolTable) {}


		llvm::Value* operator()(const parser::base_expr& expr);
		llvm::Value* operator()(const std::string& expr);
		llvm::Value* operator()(const parser::func_expr& expr);
		llvm::Value* operator()(const parser::decl_expr& expr);
		llvm::Value* operator()(const parser::operator_expr& expr);
		llvm::Value* operator()(const parser::return_expr& expr);
		llvm::Value* operator()(const parser::call_expr& expr);
		llvm::Value* operator()(const parser::if_expr& expr);
		llvm::Value* operator()(const parser::binary_op& expr);
		llvm::Value* operator()(const parser::while_loop& expr);
		llvm::Value* operator()(const parser::var_assign& expr);

	private:
		llvm::Module* m_module;
		llvm::IRBuilder<>& m_builder;

		symbolType_t m_symbolTable;
	};

}

