#include "driver.h"

#include <boost/variant/get.hpp>

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/IRBuilder.h>
#include "llvm/IR/LLVMContext.h"
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>

#include "parser.h"
#include "codegen.h"


using namespace mhc;
using namespace parser;

using namespace llvm;
using namespace std;

namespace mhc {

	namespace driver {

		bool generateOutput(const string& fileContents, const string& outputBitCodeName) {
			// Parse the source file
			//cout << "Parsing..." << endl;
			base_expr_node rootAst;
			if (!parse(fileContents, rootAst)) {
				cerr << "Failed to parse source file!" << endl;
				return false;
			}

			// Generate the code
			//cout << "Generating code..." << endl;
			LLVMContext &context = getGlobalContext();
			unique_ptr<Module> module(new Module("", context));
			IRBuilder<> builder(getGlobalContext());

			ast_codegen codeGenerator(module.get(), builder);

			// Generate code for each expression at the root level
			const base_expr* expr = boost::get<base_expr>(&rootAst);
			for (auto& itr : expr->children) {
				boost::apply_visitor(codeGenerator, itr);
			}

			// Perform an LLVM verify as a sanity check
			string errorInfo;
			raw_string_ostream errorOut(errorInfo);

			if (verifyModule(*module, &errorOut)) {
				cerr << "Failed to generate LLVM IR: " << errorInfo << endl;

				module->print(errorOut, nullptr);
				cerr << "Module:" << endl << errorInfo << endl;
				return false;
			}

			// Dump the LLVM IR to a file
			llvm::raw_fd_ostream outStream(outputBitCodeName.c_str(), errorInfo, llvm::sys::fs::F_None);
			llvm::WriteBitcodeToFile(module.get(), outStream);

			return true;
		}

		bool optimizeAndLink(const string& bitCodeFilename, const string& exeName) {
			const string tmpOptBCName = "output_opt.bc";
			const string tmpObjName = "output.o";

			// Optimize the generated bitcode with LLVM 'opt', produces an optimized bitcode file
			{
				//cout << "Optimizing..." << endl;

				const string optCmd = "opt-3.5 -filetype=obj -o " + tmpOptBCName + " -O3 -loop-unroll -loop-vectorize -slp-vectorizer output.bc";

				const int retval = system(optCmd.c_str());
				if (retval != 0) {
					cerr << "Error running 'opt': \"" << optCmd << "\"" << endl;
					return false;
				}
			}

			// Transform the bitcode into an object file with LLVM 'llc'
			{
				//cout << "Linking..." << endl;

				const string llcCmd = "llc-3.5 -filetype=obj -o " + tmpObjName + " " + tmpOptBCName;

				const int retval = system(llcCmd.c_str());
				if (retval != 0) {
					cerr << "Error running 'llc': \"" << llcCmd << "\"" << endl;
					return false;
				}
			}

			// Leverage gcc here to link the object file into the final executable
			// this is mainly to bypass the more complicated options that the system 'ld' needs
			{
				//cout << "Building executable..." << endl;

				const string outputExeName = (exeName.empty() ? "a.out" : exeName);
				const string gccCmd = "gcc -o " + outputExeName + " " + tmpObjName;
				
				const int retval = system(gccCmd.c_str());
				if (retval != 0) {
					cerr << "Error running 'gcc': \"" << gccCmd << "\"" << " -- returned: " << retval << endl;
					return false;
				}
			}

			return true;
		}

	}
}

