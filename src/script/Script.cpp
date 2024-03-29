#include "Script.hpp"

#include <script/compiler/Module.hpp>
#include <script/compiler/SemanticAnalyzer.hpp>
#include <script/compiler/Optimizer.hpp>
#include <script/vm/BytecodeStream.hpp>
#include <script/compiler/ast/AstModuleDeclaration.hpp>
#include <script/compiler/emit/Instruction.hpp>
#include <script/compiler/Lexer.hpp>
#include <script/compiler/Parser.hpp>
#include <script/compiler/Compiler.hpp>
#include <script/compiler/dis/DecompilationUnit.hpp>
#include <script/compiler/emit/codegen/CodeGenerator.hpp>
#include <script/compiler/dis/DecompilationUnit.hpp>
#include <script/compiler/builtins/Builtins.hpp>

#include <script/ScriptBindings.hpp>
#include <script/ScriptApi.hpp>

#include <script/vm/VM.hpp>

namespace hyperion {

Script::Script(const SourceFile &source_file)
    : m_api_instance(source_file),
      m_vm(m_api_instance),
      m_source_file(source_file)
{
}

Script::~Script() = default;

bool Script::Compile(scriptapi2::Context &context)
{
    if (!m_source_file.IsValid()) {
        DebugLog(
            LogType::Error,
            "Source file not valid\n"
        );

        return false;
    }

    SourceStream source_stream(&m_source_file);

    TokenStream token_stream(TokenStreamInfo {
        m_source_file.GetFilePath()
    });

    Lexer lex(source_stream, &token_stream, &m_compilation_unit);
    lex.Analyze();

    AstIterator ast_iterator;

    SemanticAnalyzer semantic_analyzer(&ast_iterator, &m_compilation_unit);

    m_compilation_unit.GetBuiltins().Visit(&semantic_analyzer);

    // Generate script bindings into our Context for our C++ classes
    g_script_bindings.GenerateAll(context);

    context.Visit(&semantic_analyzer, &m_compilation_unit);

    Parser parser(&ast_iterator, &token_stream, &m_compilation_unit);
    parser.Parse();

    semantic_analyzer.Analyze();

    m_errors = m_compilation_unit.GetErrorList();
    m_errors.WriteOutput(std::cout);

    if (!m_errors.HasFatalErrors()) {
        // only optimize if there were no errors
        // before this point
        ast_iterator.ResetPosition();

        Optimizer optimizer(&ast_iterator, &m_compilation_unit);
        optimizer.Optimize();

        // compile into bytecode instructions
        ast_iterator.ResetPosition();

        Compiler compiler(&ast_iterator, &m_compilation_unit);

        // if (auto builtins_result = builtins.Build(&m_compilation_unit)) {            
        //     m_bytecode_chunk.Append(std::move(builtins_result));
        // } else {
        //     DebugLog(
        //         LogType::Error,
        //         "Failed to add builtins to script\n"
        //     );

        //     return false;
        // }

        if (auto compile_result = compiler.Compile()) {
            m_bytecode_chunk.Append(std::move(compile_result));
        } else {
            DebugLog(
                LogType::Error,
                "Failed to compile source file\n"
            );

            return false;
        }

        return true;
    }

    return false;
}

InstructionStream Script::Decompile(utf::utf8_ostream *os) const
{
    AssertThrow(IsCompiled() && IsBaked());

    BytecodeStream bytecode_stream(m_baked_bytes.Data(), m_baked_bytes.Size());

    return DecompilationUnit().Decompile(bytecode_stream, os);
}

void Script::Bake()
{
    AssertThrow(IsCompiled());

    BuildParams build_params { };

    Bake(build_params);
}

void Script::Bake(BuildParams &build_params)
{
    AssertThrow(IsCompiled());

    CodeGenerator code_generator(build_params);
    code_generator.Visit(&m_bytecode_chunk);
    code_generator.Bake();

    m_baked_bytes = code_generator.GetInternalByteStream().GetData();

    m_bs = BytecodeStream(m_baked_bytes.Data(), m_baked_bytes.Size());
}

void Script::Run(scriptapi2::Context &context)
{
    AssertThrow(IsCompiled() && IsBaked());

    // bad things will happen if we don't set the VM
    m_api_instance.SetVM(&m_vm);
    
    context.BindAll(m_api_instance, &m_vm);
    m_vm.Execute(&m_bs);
}

void Script::CallFunctionArgV(const FunctionHandle &handle, Value *args, ArgCount num_args)
{
    AssertThrow(IsCompiled() && IsBaked());

    auto *main_thread = m_vm.GetState().GetMainThread();

    if (num_args != 0) {
        AssertThrow(args != nullptr);

        for (ArgCount i = 0; i < num_args; i++) {
            main_thread->m_stack.Push(args[i]);
        }
    }

    m_vm.InvokeNow(
        &m_bs,
        handle._inner,
        num_args
    );

    if (num_args != 0) {
        main_thread->m_stack.Pop(num_args);
    }
}

} // namespace hyperion
