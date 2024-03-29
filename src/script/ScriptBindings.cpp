#include <script/ScriptBindings.hpp>
#include <script/ScriptBindingDef.generated.hpp>
#include <script/vm/VMMemoryBuffer.hpp>

#include <asset/BufferedByteReader.hpp>

#include <script/compiler/ast/AstFloat.hpp>
#include <script/compiler/ast/AstString.hpp>
#include <script/compiler/dis/DecompilationUnit.hpp>

#include <script/Script.hpp>

#include <math/MathUtil.hpp>
 
#include <core/lib/TypeMap.hpp>

namespace hyperion {

using namespace vm;
using namespace compiler;

struct FilePointerMap
{
    HashMap<UInt32, FILE *> data;
    UInt32 counter = 0;
};

thread_local FilePointerMap file_pointer_map;

APIInstance::ClassBindings ScriptBindings::class_bindings = {};
// static APIInstance::ClassBindings class_bindings = {};

void ScriptBindings::DeclareAll(APIInstance &api_instance)
{
    using namespace hyperion::compiler;

#if 0
    api_instance.Module(Config::global_module_name)
        .Class<Name>(
            "Name",
            {
                API::NativeMemberDefine("hash_code", BuiltinTypes::UNSIGNED_INT, vm::Value(vm::Value::U64, { .u64 = 0 })),

                API::NativeMemberDefine(
                    "LookupString",
                    BuiltinTypes::STRING,
                    {
                        { "self", BuiltinTypes::ANY }
                    },
                    NameToString
                ),

                API::NativeMemberDefine(
                    "$construct",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "str", BuiltinTypes::STRING, RC<AstString>(new AstString("", SourceLocation::eof)) }
                    },
                    NameCreateFromString
                )
            }
        );

    api_instance.Module(Config::global_module_name)
        .Class<RC<DynModule>>(
            "Module",
            {
                API::NativeMemberDefine(
                    "Get",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "name", BuiltinTypes::STRING }
                    },
                    GetModuleExportedValue
                )
            },
            {
                API::NativeMemberDefine(
                    "Load",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::CLASS_TYPE },
                        { "name", BuiltinTypes::STRING }
                    },
                    LoadModule
                )
            }
        );

    api_instance.Module("vm")
        .Function(
            "ReadStackVar",
            BuiltinTypes::ANY,
            {
                { "index", BuiltinTypes::UNSIGNED_INT }
            },
            VM_ReadStackVar
        );

    api_instance.Module(Config::global_module_name)
        .Function(
            "MakeStruct",
            BuiltinTypes::ANY,
            {
                {
                    "members",
                    BuiltinTypes::ANY
                }
            },
            Runtime_MakeStruct
        )
        .Function(
            "GetStructMember",
            BuiltinTypes::ANY,
            {
                {
                    "struct",
                    BuiltinTypes::ANY
                },
                {
                    "member_name",
                    BuiltinTypes::STRING
                }
            },
            Runtime_GetStructMember
        )
        .Function(
            "SetStructMember",
            BuiltinTypes::BOOLEAN,
            {
                {
                    "struct",
                    BuiltinTypes::ANY
                },
                {
                    "member_name",
                    BuiltinTypes::STRING
                },
                {
                    "value",
                    BuiltinTypes::ANY
                }
            },
            Runtime_SetStructMember
        )
        .Function(
            "GetStructMemoryBuffer",
            BuiltinTypes::ANY,
            {
                {
                    "struct",
                    BuiltinTypes::ANY
                }
            },
            Runtime_GetStructMemoryBuffer
        )
        .Function(
            "fopen",
            BuiltinTypes::UNSIGNED_INT,
            {
                {
                    "path",
                    BuiltinTypes::STRING
                },
                {
                    "args",
                    BuiltinTypes::STRING
                }
            },
            Runtime_OpenFilePointer
        )
        .Function(
            "fclose",
            BuiltinTypes::BOOLEAN,
            {
                {
                    "file_id",
                    BuiltinTypes::UNSIGNED_INT
                }
            },
            Runtime_CloseFilePointer
        )
        .Function(
            "fwrite",
            BuiltinTypes::VOID_TYPE,
            {
                {
                    "file_id",
                    BuiltinTypes::UNSIGNED_INT
                },
                {
                    "data",
                    BuiltinTypes::ANY
                }
            },
            Runtime_WriteFileData
        )
        .Function(
            "fflush",
            BuiltinTypes::VOID_TYPE,
            {
                {
                    "file_id",
                    BuiltinTypes::UNSIGNED_INT
                }
            },
            Runtime_FlushFileStream
        )
        .Function(
            "ToMemoryBuffer",
            BuiltinTypes::ANY,
            {
                {
                    "obj",
                    BuiltinTypes::ANY
                }
            },
            Runtime_ToMemoryBuffer
        )
        .Function(
            "GetMemoryAddress",
            BuiltinTypes::STRING,
            {
                { "value", BuiltinTypes::ANY }
            },
            Runtime_GetMemoryAddress
        )
        .Function(
            "GetFunctionBytecode",
            BuiltinTypes::STRING,
            {
                { "value", BuiltinTypes::ANY }
            },
            Runtime_GetFunctionBytecode
        )
        .Function(
            "GetTypeString",
            BuiltinTypes::STRING,
            {
                { "value", BuiltinTypes::ANY }
            },
            Runtime_GetTypeString
        )
        .Function(
            "IsInstance",
            BuiltinTypes::BOOLEAN,
            {
                { "target", BuiltinTypes::ANY },
                { "cls", BuiltinTypes::ANY },
            },
            Runtime_IsInstance
        )
        .Function(
            "GetClass",
            BuiltinTypes::CLASS_TYPE,
            {
                { "object", BuiltinTypes::ANY }
            },
            Runtime_GetClass
        )
        .Function(
            "HasMember",
            BuiltinTypes::BOOLEAN,
            {
                { "object", BuiltinTypes::ANY },
                { "member_name", BuiltinTypes::STRING }
            },
            Runtime_HasMember
        )
        .Function(
            "GetMember",
            BuiltinTypes::ANY,
            {
                { "object", BuiltinTypes::ANY },
                { "member_name", BuiltinTypes::STRING }
            },
            Runtime_GetMember
        )
        .Function(
            "SetMember",
            BuiltinTypes::VOID_TYPE,
            {
                { "object", BuiltinTypes::ANY },
                { "member_name", BuiltinTypes::STRING },
                { "value", BuiltinTypes::ANY }
            },
            Runtime_SetMember
        )
        .Function(
            "GetMembers",
            BuiltinTypes::ANY,
            {
                { "object", BuiltinTypes::ANY }
            },
            Runtime_GetMembers
        );

    api_instance.Module(Config::global_module_name)
        .Function(
            "Engine_CreateEntity",
            BuiltinTypes::ANY,
            {
                { "engine", BuiltinTypes::ANY },
            },
            EngineCreateEntity
        );

    api_instance.Module(Config::global_module_name)
        .Class<Vec2f>(
            "Vec2f",
            {
                API::NativeMemberDefine(
                    "$construct",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "x", BuiltinTypes::FLOAT, RC<AstFloat>(new AstFloat(0.0f, SourceLocation::eof)) },
                        { "y", BuiltinTypes::FLOAT, RC<AstFloat>(new AstFloat(0.0f, SourceLocation::eof)) }
                    },
                    CxxCtor< Vec2f, Float, Float > 
                ),
                API::NativeMemberDefine(
                    "operator+",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec2f, Vec2f, const Vec2f &, &Vec2f::operator+ >
                ),
                API::NativeMemberDefine(
                    "operator+=",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec2f &, Vec2f, const Vec2f &, &Vec2f::operator+= >
                ),
                API::NativeMemberDefine(
                    "operator-",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec2f, Vec2f, const Vec2f &, &Vec2f::operator- >
                ),
                API::NativeMemberDefine(
                    "operator-=",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec2f &, Vec2f, const Vec2f &, &Vec2f::operator-= >
                ),
                API::NativeMemberDefine(
                    "operator*",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec2f, Vec2f, const Vec2f &, &Vec2f::operator* >
                ),
                API::NativeMemberDefine(
                    "operator*=",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec2f &, Vec2f, const Vec2f &, &Vec2f::operator*= >
                ),
                API::NativeMemberDefine(
                    "operator/",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec2f, Vec2f, const Vec2f &, &Vec2f::operator/ >
                ),
                API::NativeMemberDefine(
                    "operator/=",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec2f &, Vec2f, const Vec2f &, &Vec2f::operator/= >
                ),
                API::NativeMemberDefine(
                    "operator==",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< bool, Vec2f, const Vec2f &, &Vec2f::operator== >
                ),
                API::NativeMemberDefine(
                    "operator!=",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< bool, Vec2f, const Vec2f &, &Vec2f::operator!= >
                ),
                API::NativeMemberDefine(
                    "Length",
                    BuiltinTypes::FLOAT,
                    {
                        { "self", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Float, Vec2f, &Vec2f::Length >
                ),
                API::NativeMemberDefine(
                    "Distance",
                    BuiltinTypes::FLOAT,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Float, Vec2f, const Vec2f &, &Vec2f::Distance >
                ),
                API::NativeMemberDefine(
                    "Normalize",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec2f &, Vec2f, &Vec2f::Normalize >
                ),
                API::NativeMemberDefine(
                    "GetX",
                    BuiltinTypes::FLOAT,
                    {
                        { "self", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Float, Vec2f, &Vec2f::GetX >
                ),
                API::NativeMemberDefine(
                    "GetY",
                    BuiltinTypes::FLOAT,
                    {
                        { "self", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Float, Vec2f, &Vec2f::GetY >
                )
            }
        );

    api_instance.Module(Config::global_module_name)
        .Class<Vec3f>(
            "Vec3f",
            {
                API::NativeMemberDefine(
                    "$construct",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "x", BuiltinTypes::FLOAT, RC<AstFloat>(new AstFloat(0.0f, SourceLocation::eof)) },
                        { "y", BuiltinTypes::FLOAT, RC<AstFloat>(new AstFloat(0.0f, SourceLocation::eof)) },
                        { "z", BuiltinTypes::FLOAT, RC<AstFloat>(new AstFloat(0.0f, SourceLocation::eof)) }
                    },
                    CxxCtor< Vec3f, Float, Float, Float > 
                ),
                API::NativeMemberDefine(
                    "operator+",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec3f, Vec3f, const Vec3f &, &Vec3f::operator+ >
                ),
                API::NativeMemberDefine(
                    "operator+=",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec3f &, Vec3f, const Vec3f &, &Vec3f::operator+= >
                ),
                API::NativeMemberDefine(
                    "operator-",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec3f, Vec3f, const Vec3f &, &Vec3f::operator- >
                ),
                API::NativeMemberDefine(
                    "operator-=",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec3f &, Vec3f, const Vec3f &, &Vec3f::operator-= >
                ),
                API::NativeMemberDefine(
                    "operator*",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec3f, Vec3f, const Vec3f &, &Vec3f::operator* >
                ),
                API::NativeMemberDefine(
                    "operator*=",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec3f &, Vec3f, const Vec3f &, &Vec3f::operator*= >
                ),
                API::NativeMemberDefine(
                    "operator/",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec3f, Vec3f, const Vec3f &, &Vec3f::operator/ >
                ),
                API::NativeMemberDefine(
                    "operator/=",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec3f &, Vec3f, const Vec3f &, &Vec3f::operator/= >
                ),
                API::NativeMemberDefine(
                    "operator==",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< bool, Vec3f, const Vec3f &, &Vec3f::operator== >
                ),
                API::NativeMemberDefine(
                    "operator!=",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< bool, Vec3f, const Vec3f &, &Vec3f::operator!= >
                ),
                API::NativeMemberDefine(
                    "Dot",
                    BuiltinTypes::FLOAT,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Float, Vec3f, const Vec3f &, &Vec3f::Dot >
                ),
                API::NativeMemberDefine(
                    "Cross",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec3f, Vec3f, const Vec3f &, &Vec3f::Cross >
                ),
                API::NativeMemberDefine(
                    "Length",
                    BuiltinTypes::FLOAT,
                    {
                        { "self", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Float, Vec3f, &Vec3f::Length >
                ),
                API::NativeMemberDefine(
                    "Distance",
                    BuiltinTypes::FLOAT,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "other", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Float, Vec3f, const Vec3f &, &Vec3f::Distance >
                ),
                API::NativeMemberDefine(
                    "Normalized",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec3f, Vec3f, &Vec3f::Normalized >
                ),
                API::NativeMemberDefine(
                    "Normalize",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Vec3f &, Vec3f, &Vec3f::Normalize >
                ),
                API::NativeMemberDefine(
                    "ToString",
                    BuiltinTypes::STRING,
                    {
                        { "self", BuiltinTypes::ANY }
                    },
                    Vector3ToString
                ),
                API::NativeMemberDefine(
                    "GetX",
                    BuiltinTypes::FLOAT,
                    {
                        { "self", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Float, Vec3f, &Vec3f::GetX >
                ),
                API::NativeMemberDefine(
                    "GetY",
                    BuiltinTypes::FLOAT,
                    {
                        { "self", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Float, Vec3f, &Vec3f::GetY >
                ),
                API::NativeMemberDefine(
                    "GetZ",
                    BuiltinTypes::FLOAT,
                    {
                        { "self", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< Float, Vec3f, &Vec3f::GetZ >
                )
                // {
                //     "SetX",
                //     BuiltinTypes::ANY,
                //     {
                //         { "self", BuiltinTypes::ANY },
                //         { "value", BuiltinTypes::FLOAT }
                //     },
                //     CxxMemberFn< Vec3f &, Vec3f, Float, &Vec3f::SetX >
                // },
                // {
                //     "SetY",
                //     BuiltinTypes::ANY,
                //     {
                //         { "self", BuiltinTypes::ANY },
                //         { "value", BuiltinTypes::FLOAT }
                //     },
                //     CxxMemberFn< Vec3f &, Vec3f, Float, &Vec3f::SetY >
                // },
                // {
                //     "SetZ",
                //     BuiltinTypes::ANY,
                //     {
                //         { "self", BuiltinTypes::ANY },
                //         { "value", BuiltinTypes::FLOAT }
                //     },
                //     CxxMemberFn< Vec3f &, Vec3f, Float, &Vec3f::SetZ >
                // },
                // {
                //     "operator+",
                //     BuiltinTypes::ANY,
                //     {
                //         { "self", BuiltinTypes::ANY },
                //         { "other", BuiltinTypes::ANY }
                //     },
                //     CxxMemberFn< Vec3f, Vec3f, const Vec3f &, &Vec3f::operator+ >
                // },
                // {
                //     "operator-",
                //     BuiltinTypes::ANY,
                //     {
                //         { "self", BuiltinTypes::ANY },
                //         { "other", BuiltinTypes::ANY }
                //     },
                //     CxxMemberFn< Vec3f, Vec3f, const Vec3f &, &Vec3f::operator- >
                // }
            }
        );

    api_instance.Module(Config::global_module_name)
        .Class<BoundingBox>(
            "BoundingBox",
            {
                API::NativeMemberDefine(
                    "$construct",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY },
                        { "min", BuiltinTypes::ANY },
                        { "max", BuiltinTypes::ANY }
                    },
                    CxxCtor< BoundingBox, Vec3f, Vec3f > 
                ),
                API::NativeMemberDefine(
                    "GetMin",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< const Vec3f &, BoundingBox, &BoundingBox::GetMin >
                ),
                API::NativeMemberDefine(
                    "GetMax",
                    BuiltinTypes::ANY,
                    {
                        { "self", BuiltinTypes::ANY }
                    },
                    CxxMemberFn< const Vec3f &, BoundingBox, &BoundingBox::GetMax >
                ),
            }
        );

    api_instance.Module(Config::global_module_name)
        .Variable("SCRIPT_VERSION", 200)
        .Variable("ENGINE_VERSION", 200)
#ifdef HYP_DEBUG_MODE
        .Variable("DEBUG_MODE", true)
#else
        .Variable("DEBUG_MODE", false)
#endif
        .Variable("NaN", MathUtil::NaN<Float>())
        .Function(
            "__array_size",
            BuiltinTypes::INT,
            {
                { "self", BuiltinTypes::ANY } // one of: VMArray, VMString, VMObject
            },
            ArraySize
        )
        .Function(
            "__array_push",
            BuiltinTypes::ANY,
            {
                { "self", BuiltinTypes::ANY },
                {
                    "args",
                    SymbolType::GenericInstance(
                        BuiltinTypes::VAR_ARGS,
                        GenericInstanceTypeInfo {
                            {
                                { "arg", BuiltinTypes::ANY }
                            }
                        }
                    )
                }
            },
            ArrayPush
        )
        .Function(
            "__array_pop",
            BuiltinTypes::ANY, // returns object that was popped
            {
                { "self", BuiltinTypes::ANY }
            },
            ArrayPop
        )
        .Function(
            "__array_get",
            BuiltinTypes::ANY,
            {
                { "self", BuiltinTypes::ANY },
                { "index", BuiltinTypes::INT }
            },
            ArrayGet
        )
        .Function(
            "__array_set",
            BuiltinTypes::ANY,
            {
                { "self", BuiltinTypes::ANY },
                { "index", BuiltinTypes::INT },
                { "value", BuiltinTypes::ANY }
            },
            ArraySet
        )

        // hashcode
        .Function(
            "__hash",
            BuiltinTypes::UNSIGNED_INT,
            {
                { "self", BuiltinTypes::ANY }
            },
            GetHashCode
        )

        // map functions
        .Function(
            "__map_new",
            BuiltinTypes::ANY,
            {
            },
            MapNew
        )
        .Function(
            "__map_get",
            BuiltinTypes::ANY,
            {
                { "self", BuiltinTypes::ANY },
                { "key", BuiltinTypes::ANY }
            },
            MapGet
        )
        .Function(
            "__map_set",
            BuiltinTypes::ANY,
            {
                { "self", BuiltinTypes::ANY },
                { "key", BuiltinTypes::ANY },
                { "value", BuiltinTypes::ANY }
            },
            MapSet
        )

        .Function(
            "Puts",
            BuiltinTypes::INT,
            {
                { "str", BuiltinTypes::STRING }
            },
            Puts
        )
        .Function(
            "ToString",
            BuiltinTypes::STRING,
            {
                { "obj", BuiltinTypes::ANY }
            },
            ToString
        )
        .Function(
            "Format",
            BuiltinTypes::STRING,
            {
                { "format", BuiltinTypes::STRING },
                {
                    "args",
                    SymbolType::GenericInstance(
                        BuiltinTypes::VAR_ARGS,
                        GenericInstanceTypeInfo {
                            {
                                { "arg", BuiltinTypes::ANY }
                            }
                        }
                    )
                }
            },
            Format
        )
        .Function(
            "Print",
            BuiltinTypes::INT,
            {
                { "format", BuiltinTypes::STRING },
                {
                    "args",
                    SymbolType::GenericInstance(
                        BuiltinTypes::VAR_ARGS,
                        GenericInstanceTypeInfo {
                            {
                                { "arg", BuiltinTypes::ANY }
                            }
                        }
                    )
                }
            },
            Print
        )
        .Function(
            "Malloc",
            BuiltinTypes::ANY,
            {
                { "size", BuiltinTypes::INT } // TODO: should be unsigned, but having conversion issues
            },
            Malloc
        )
        .Function(
            "Free",
            BuiltinTypes::VOID_TYPE,
            {
                { "ptr", BuiltinTypes::ANY } // TODO: should be unsigned, but having conversion issues
            },
            Free
        );
#endif
}

} // namespace hyperion
