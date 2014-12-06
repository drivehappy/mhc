#include "codegen.h"

#include <map>
#include <string>
#include <vector>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>

// Debugging
#include <iostream>


using namespace marklar;
using namespace parser;

using namespace llvm;
using namespace std;
using namespace std::placeholders;


namespace {
	bool is_number(const string& s) {
		return !s.empty() && find_if(s.begin(),
			s.end(), [](char c) { return !isdigit(c); }) == s.end();
	}
}


Value* ast_codegen::operator()(const string& val) {
	//cerr << "Generating code for string \"" << val << "\"" << endl;

	BasicBlock *bb = m_builder.GetInsertBlock();
	Function *TheFunction = bb->getParent();
	const string varName = string(TheFunction->getName()) + "_" + val;

	Value *retVal = nullptr;

	map<string, Value*>::iterator itr = m_symbolTable.find(varName);
	if (itr != m_symbolTable.end()) {
		Value* const localVar = itr->second;

		// Only create a load if this is a pointer type, this avoids
		// problems with function arguments that aren't created through Alloca
		if (localVar->getType()->isPointerTy()) {
			retVal = m_builder.CreateLoad(localVar);
		} else {
			retVal = localVar;
		}
	} else if (is_number(val)) {
		APInt vInt(64, stol(val));
		retVal = ConstantInt::get(getGlobalContext(), vInt);
	} else {
		cerr << "ERROR: Could not find symbol: \"" << val << "\"" << endl;
		cerr << "  SymbolTable size: " << m_symbolTable.size() << endl;

		for (const auto& kv : m_symbolTable) {
			cerr  << "    Key: " << kv.first << ", Value: " << kv.second << endl;
		}

		cerr << endl;
	}

	return retVal;
}

Value* ast_codegen::operator()(const parser::base_expr& expr) {
	//cerr << "Generating code for base_expr, children size = \"" << expr.children.size() << "\"" << endl;

	//assert(expr);

	/*
	for (base_expr_node& itr : expr->children) {
		boost::apply_visitor(ast_codegen(m_module, m_builder), &itr));
	}
	*/

	return nullptr;
}

Value* ast_codegen::operator()(const parser::func_expr& func) {
	//cerr << "Generating code for Function \"" << func.functionName << "\"" << endl;

	Function *F = nullptr;

	// Determine if this function name has been defined yet
	auto itr = m_symbolTable.find(func.functionName);
	if (itr == m_symbolTable.end()) {
		// Could not find existing function with this name, build it
		vector<Type*> args(func.args.size(), Type::getInt64Ty(getGlobalContext()));
		FunctionType *FT = FunctionType::get(Type::getInt64Ty(getGlobalContext()), args, false);
		F = Function::Create(FT, Function::ExternalLinkage, func.functionName, m_module);

		// Add it to the symbol table so we can refer to it later
		m_symbolTable[func.functionName] = F;
	} else {
		F = dynamic_cast<Function*>(itr->second);
	}

	BasicBlock *BB = BasicBlock::Create(getGlobalContext(), func.functionName.c_str(), F);
	m_builder.SetInsertPoint(BB);

	// Build a return value in place
	IRBuilder<> TmpB(&F->getEntryBlock(), F->getEntryBlock().begin());
	AllocaInst* const Alloca = TmpB.CreateAlloca(Type::getInt64Ty(getGlobalContext()), nullptr, "__retval__");
	assert(Alloca);
	m_symbolTable["__retval__"] = Alloca;

	APInt vInt(64, 0);
	m_builder.CreateStore(ConstantInt::get(getGlobalContext(), vInt), Alloca);

	BasicBlock *ReturnBB = BasicBlock::Create(getGlobalContext(), "return");
	m_symbolTable["__retval__BB"] = ReturnBB;


	// Add function arguments
	Function::arg_iterator argItr = F->arg_begin();
	for (auto& argStr : func.args) {
		const string argName = string(F->getName()) + "_" + argStr;

		argItr->setName(argName);
		m_symbolTable[argName] = argItr;

		++argItr;
	}

	// Create a new visitor, this allows function-level scoping so our symbol table
	// isn't re-used across other functions
	ast_codegen symbolVisitor(*this);

	// Visit declarations inside the function node
	for (auto& itrDecl : func.declarations) {
		boost::apply_visitor(symbolVisitor, itrDecl);
	}

	// Default this to something other than nullptr, in cases where we
	// don't have a return we may not actually generate a return stmt below
	Value* lastExpr = ReturnBB;

	// Visit expressions inside the function node
	for (auto& itrExpr : func.expressions) {
		lastExpr = boost::apply_visitor(symbolVisitor, itrExpr);

		// Indicates that all paths branched (in the current case, this means everything returned)
		// and we didn't create an "if.end" merge block, therefore stop here
		if (!lastExpr) {
			break;
		}

		if ((lastExpr && isa<BranchInst>(lastExpr))) {
			break;
		}
	}

	// Build the return inst
	F->getBasicBlockList().push_back(ReturnBB);
	m_builder.SetInsertPoint(ReturnBB);

	Value* const loadRetVal = m_builder.CreateLoad(m_symbolTable["__retval__"]);
	assert(loadRetVal);
	Value* const retVal = m_builder.CreateRet(loadRetVal);
	assert(retVal);

	// LLVM sanity check
	verifyFunction(*F);

	return nullptr;
}

Value* ast_codegen::operator()(const parser::decl_expr& decl) {
	//cerr << "Generating code for declaration \"" << decl.declName << "\"" << endl;

	Value* var = nullptr;

	BasicBlock *bb = m_builder.GetInsertBlock();
	Function *TheFunction = bb->getParent();

	const string declName = string(TheFunction->getName()) + "_" + decl.declName;

	map<string, Value*>::const_iterator itr = m_symbolTable.find(declName);
	if (itr == m_symbolTable.end()) {
		//cerr << "  Variable referenced for first time: " << decl.declName << endl;

		AllocaInst *Alloca = nullptr;

		// If there is no basic block it indicates it might be at the global-level
		if (bb) {
			IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
			Alloca = TmpB.CreateAlloca(Type::getInt64Ty(getGlobalContext()), nullptr, declName.c_str());

			m_symbolTable[declName] = Alloca;
		} else {
			// Check if the function was already declared, if not then build it
			auto itr = m_symbolTable.find(declName);
			if (itr == m_symbolTable.end()) {
				// Assume for now this has no arguments
				vector<Type*> args(0, Type::getInt64Ty(getGlobalContext()));

				FunctionType *FT = FunctionType::get(Type::getInt64Ty(getGlobalContext()), args, false);
				Function *F = Function::Create(FT, Function::ExternalLinkage, declName, m_module);

				// Add this function to the symbol table
				m_symbolTable[declName] = F;
			}
		}

		var = Alloca;
	} else {
		// Use the variable itself
		var = itr->second;
	}

	Value* const exprRhs = boost::apply_visitor(*this, decl.val);
	if (exprRhs) {
		// Don't just obtain the variable from codegen, since that produces a load...
		// instead just look it up directly
		const auto itr = m_symbolTable.find(declName);
		if (itr != m_symbolTable.end()) {
			if (exprRhs->getType()->isPointerTy()) {
				Value *varLhs = m_builder.CreateLoad(exprRhs);
				m_builder.CreateStore(varLhs, itr->second);
			} else {
				m_builder.CreateStore(exprRhs, itr->second);
			}
		} else {
			cerr << "ERROR: Could not find variable: " << declName << endl;
			return nullptr;
		}
	}

	return var;
}

Value* ast_codegen::operator()(const parser::operator_expr& expr) {
	//cerr << "Generating code for operator" << endl;

	return nullptr;
}

Value* ast_codegen::operator()(const parser::return_expr& exprRet) {
	//cerr << "Generating code for return:" << endl;
	
	// We can't generate a CreateRet in-place here since it might be
	// within an if-else, LLVM doesn't allow terminators in the branches
	// Therefore, just reference the return value on the stack we setup
	// when the function was created - we should only be writing to this once
	Value* const retVal = m_symbolTable["__retval__"];
	assert(retVal);

#if 0	
	if (m_returnGenerated) {
		/*
		APInt vInt(64, stoi(val));
		defaultRet = ConstantInt::get(getGlobalContext(), vInt);

		m_builder.CreateStore(defaultRet, retVal);
		*/
		return retVal;
	}
#endif

	Value* const v = boost::apply_visitor(*this, exprRet.ret);
	assert(v);

	Value* const n = m_builder.CreateStore(v, retVal);
	assert(n);

	Value* const ReturnBB = m_symbolTable["__retval__BB"];
	Value* const r = m_builder.CreateBr(dynamic_cast<BasicBlock*>(ReturnBB));
	assert(r);

	// This should be a pointer type, the function would have created this
	//assert(retVal->getType()->isPointerTy());

	//m_returnGenerated = true;

	return r;
}

Value* ast_codegen::operator()(const parser::call_expr& expr) {
	//cerr << "Generating code for call_expr:" << endl;

	const string& callFuncName = expr.funcName;
	Function *calleeF = m_module->getFunction(callFuncName);
	if (calleeF == nullptr) {
		cerr << "Error: Could not find function definition for \"" << callFuncName << "\"" << endl;
		return nullptr;
	}

	// Check that the number of arguments match with what we expect on the function
	if (calleeF->arg_size() != expr.values.size()) {
		cerr << "Error: Function call expected " << calleeF->arg_size() << " arguments, but got " << expr.values.size() << endl;
		return nullptr;
	}

	// Build the arguments
	std::vector<Value*> ArgsV;
	//cerr << "  Building call arguments: " << expr.values.size() << endl;
	for (auto& exprArg : expr.values) {
		Value* const v = boost::apply_visitor(*this, exprArg);
		ArgsV.push_back(v);
	}

	CallInst *callInst = m_builder.CreateCall(calleeF, ArgsV, callFuncName);

	// Pass the call up so the value can be stored
	return callInst;
}

Value* ast_codegen::operator()(const parser::if_expr& expr) {
	//cerr << "Generating code for ifExpr:" << endl;

	Function *TheFunction = m_builder.GetInsertBlock()->getParent();
	assert(TheFunction);

	// Call the visitor directory for the binary_op
	Value* const CondV = (*this)(expr.condition);
	assert(CondV);

	// Create blocks for the then and else cases, insert the 'then' block at the
	// end of the function
	BasicBlock *ThenBB = BasicBlock::Create(getGlobalContext(), "if.then", TheFunction);
	BasicBlock *ElseBB = BasicBlock::Create(getGlobalContext(), "if.else");
	BasicBlock *MergeBB = BasicBlock::Create(getGlobalContext(), "if.end");

	m_builder.CreateCondBr(CondV, ThenBB, ElseBB);

	Value* ThenV = nullptr;
	Value* ElseV = nullptr;

	bool thenBranched = false;
	bool elseBranched = false;

	// Build the 'then' branch code
	{
		m_builder.SetInsertPoint(ThenBB);

		// Created a new visitor, this allows scoping so our symbol table
		// isn't re-used across other functions
		ast_codegen symbolVisitor(*this);

		for (const auto& itrThen : expr.thenBranch) {
			ThenV = boost::apply_visitor(symbolVisitor, itrThen);
			assert(ThenV);

			if (isa<BranchInst>(ThenV)) {
				break;
			}
		}

		// Create a branch to the MergeBB if the last was a branch instruction
		if (!isa<BranchInst>(ThenV)) {
			m_builder.CreateBr(MergeBB);
		} else {
			thenBranched = true;
		}

		ThenBB = m_builder.GetInsertBlock();
	}

	// Build the 'else' branch code
	{
		TheFunction->getBasicBlockList().push_back(ElseBB);
		m_builder.SetInsertPoint(ElseBB);

		// Created a new visitor, this allows scoping so our symbol table
		// isn't re-used across other functions
		ast_codegen symbolVisitor(*this);

		for (const auto& itrElse : expr.elseBranch) {
			ElseV = boost::apply_visitor(symbolVisitor, itrElse);
			assert(ElseV);

			if (isa<BranchInst>(ElseV)) {
				break;
			}
		}

		// Create a branch to the MergeBB if the last was a branch instruction
		if (!ElseV || (ElseV && !isa<BranchInst>(ElseV))) {
			m_builder.CreateBr(MergeBB);
		} else {
			elseBranched = true;
		}

		ElseBB = m_builder.GetInsertBlock();
	}

	// Finalize the condition, sets the insertion point so code after this is
	// attached to the "if.end" block, only if the then and else branches both didn't branch
	if (thenBranched && elseBranched) {
		return nullptr;
	} else {
		TheFunction->getBasicBlockList().push_back(MergeBB);
		m_builder.SetInsertPoint(MergeBB);

		return MergeBB;
	}
}

Value* ast_codegen::operator()(const parser::binary_op& op) {
	//cerr << "Generating code for binary_op:" << endl;

	typedef Value* (IRBuilder<>::*logical_t)(Value*, Value*, const Twine&);
	typedef Value* (IRBuilder<>::*shiftRight_t)(Value*, Value*, const Twine&, bool);
	typedef Value* (IRBuilder<>::*shiftLeft_t)(Value*, Value*, const Twine&, bool, bool);

	// Mapping of operator to LLVM creation calls
	const map<string, std::function<Value*(Value*, Value*)>> ops = {
		{ "+",  bind(&IRBuilder<>::CreateAdd,     m_builder, _1, _2, "add", false, false) },
		{ "-",  bind(&IRBuilder<>::CreateSub,     m_builder, _1, _2, "sub", false, false) },
		{ "<",  bind(&IRBuilder<>::CreateICmpSLT, m_builder, _1, _2, "cmp") },
		{ ">",  bind(&IRBuilder<>::CreateICmpSGT, m_builder, _1, _2, "cmp") },
		{ "%",  bind(&IRBuilder<>::CreateSRem,    m_builder, _1, _2, "rem") },
		{ "/",  bind(&IRBuilder<>::CreateSDiv,    m_builder, _1, _2, "div", false) },
		{ "*",  bind(&IRBuilder<>::CreateMul,     m_builder, _1, _2, "mult", false, false) },
		{ ">=", bind(&IRBuilder<>::CreateICmpSGE, m_builder, _1, _2, "cmp") },
		{ "<=", bind(&IRBuilder<>::CreateICmpSLE, m_builder, _1, _2, "cmp") },
		{ "==", bind(&IRBuilder<>::CreateICmpEQ,  m_builder, _1, _2, "cmp") },
		{ "!=", bind(&IRBuilder<>::CreateICmpNE,  m_builder, _1, _2, "cmp") },
		{ "&",  bind(static_cast<logical_t>
		            (&IRBuilder<>::CreateAnd),    m_builder, _1, _2, "and") },
		{ "||", bind(static_cast<logical_t>
		            (&IRBuilder<>::CreateOr),     m_builder, _1, _2, "or")  },
		{ "&&", bind(static_cast<logical_t>
		            (&IRBuilder<>::CreateAnd),    m_builder, _1, _2, "and")  },
		{ ">>", bind(static_cast<shiftRight_t>
		            (&IRBuilder<>::CreateLShr),   m_builder, _1, _2, "shr", false) },
		{ "<<", bind(static_cast<shiftLeft_t>
		            (&IRBuilder<>::CreateShl),    m_builder, _1, _2, "shl", false, false) },
	};

	Value* varLhs = boost::apply_visitor(*this, op.lhs);
	assert(varLhs);

	// This acts a chain, e.g.: "1 + 3 + i + k", varLhs is built up for each
	for (auto& itr : op.operation) {
		Value* const varRhs = boost::apply_visitor(*this, itr.rhs);
		assert(varRhs);

		const auto& itr2 = ops.find(itr.op);
		if (itr2 == ops.end()) {
			cerr << "Unknown operator: \"" << itr.op << "\"" << endl;
			assert(false && "Unsupported operator");
			return nullptr;
		}

		// Call the mapped operator type to create the appropriate one
		varLhs = itr2->second(varLhs, varRhs);
	}

	return varLhs;
}

Value* ast_codegen::operator()(const parser::while_loop& loop) {
	//cerr << "Generating code for while loop" << endl;

	Function *TheFunction = m_builder.GetInsertBlock()->getParent();

	BasicBlock *LoopBB = BasicBlock::Create(getGlobalContext(), "while.body");
	BasicBlock *AfterBB = BasicBlock::Create(getGlobalContext(), "while.end");
	BasicBlock *loopCond = BasicBlock::Create(getGlobalContext(), "while.cond", TheFunction);

	m_builder.CreateBr(loopCond);
	m_builder.SetInsertPoint(loopCond);

	// Generate the condition code directly
	Value* const cond = (*this)(loop.condition);
	m_builder.CreateCondBr(cond, LoopBB, AfterBB);

	TheFunction->getBasicBlockList().push_back(LoopBB);
	m_builder.SetInsertPoint(LoopBB);

	// Created a new visitor, this allows scoping so our symbol table isn't re-used across other functions
	ast_codegen symbolVisitor(*this);

	// Generate the loop body
	bool branchGenerated = false;
	for (const auto& itrBody : loop.loopBody) {
		Value* const v = boost::apply_visitor(symbolVisitor, itrBody);
		assert(v);

		if ((v && isa<BranchInst>(v))) {
			branchGenerated = true;
			break;
		}
	}

	if (!branchGenerated) {
		// No branches in our loop directly, go ahead and build the final block
		m_builder.CreateBr(loopCond);
	}

	TheFunction->getBasicBlockList().push_back(AfterBB);
	m_builder.SetInsertPoint(AfterBB);

	return AfterBB;
}

Value* ast_codegen::operator()(const parser::var_assign& assign) {
	//cerr << "Generating code for variable assignment of \"" << assign.varName << "\"" << endl;

	BasicBlock *bb = m_builder.GetInsertBlock();
	Function *TheFunction = bb->getParent();
	const string varName = string(TheFunction->getName()) + "_" + assign.varName;

	Value* const rhsVal = boost::apply_visitor(*this, assign.varRhs);

	const auto& itr = m_symbolTable.find(varName);
	if (itr == m_symbolTable.end()) {
		cerr << "Unknown variable assignment: \"" << assign.varName << "\"" << endl;
		return nullptr;
	}

	if (rhsVal->getType()->isPointerTy()) {
		Value* const varLhs = m_builder.CreateLoad(rhsVal);
		return m_builder.CreateStore(varLhs, itr->second);
	}
	
	return m_builder.CreateStore(rhsVal, itr->second);
}

