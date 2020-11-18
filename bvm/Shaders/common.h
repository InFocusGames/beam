// Copyright 2018 The Beam Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifndef HOST_BUILD

// Common ord types
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef signed int int32_t;
typedef signed short int16_t;
typedef signed long long int64_t;

typedef uint64_t Height;
typedef uint64_t Amount;
typedef uint32_t AssetID;
typedef uint64_t Timestamp;

#pragma pack (push, 1)

template <uint32_t nBytes>
struct Opaque {
    uint8_t m_p[nBytes];
};

#pragma pack (pop)

typedef Opaque<33> PubKey;
typedef Opaque<32> ContractID;
typedef Opaque<32> ShaderID;
typedef Opaque<32> HashValue;

template <bool bToShader, typename T>
inline void ConvertOrd(T&) {}

#   define export __attribute__( ( visibility( "default" ) ) ) extern "C"

#endif // HOST_BUILD

// environment functions
#include "../bvm2_shared.h"
#include "../bvm2_opcodes.h"

namespace Env {

    extern "C" {

#define PAR_DECL(type, name) type name
#define COMMA ,
#define THE_MACRO(id, ret, name) \
        ret name(BVMOp_##name(PAR_DECL, COMMA));

        BVMOpsAll_Common(THE_MACRO)
        BVMOpsAll_Contract(THE_MACRO)
        BVMOpsAll_Manager(THE_MACRO)

#undef THE_MACRO
#undef COMMA
#undef PAR_DECL
    } // extern "C"

    template <typename TKey, typename TVal>
    inline bool LoadVar_T(const TKey& key, TVal& val)
    {
        return LoadVar(&key, sizeof(key), &val, sizeof(val)) == sizeof(val);
    }

    template <typename TKey, typename TVal>
    inline void SaveVar_T(const TKey& key, const TVal& val)
    {
        SaveVar(&key, sizeof(key), &val, sizeof(val));
    }

    template <typename TKey>
    inline void DelVar_T(const TKey& key)
    {
        SaveVar(&key, sizeof(key), nullptr, 0);
    }

    inline void Halt_if(bool b)
    {
        if (b)
            Halt();
    }

    template <typename T>
    inline T* StackAlloc_T(uint32_t n) {
        return (T*) StackAlloc(sizeof(T) * n);
    }

    template <typename T>
    inline T* StackFree_T(uint32_t n) {
        // not mandatory to call, but sometimes usefull before calling other heavy functions
        return StackFree(sizeof(T) * n);
    }

#ifndef HOST_BUILD
    template <typename T>
    void CallFar_T(const ContractID& cid, T& args)
    {
        CallFar(cid, args.s_iMethod, &args);
    }
#endif // HOST_BUILD

    inline void DocAddNum(const char* szID, uint32_t val) {
        DocAddNum32(szID, val);
    }
    inline void DocAddNum(const char* szID, uint64_t val) {
        DocAddNum64(szID, val);
    }

    inline bool DocGet(const char* szID, uint32_t& val) {
        val = 0;
        return DocGetNum32(szID, &val) == sizeof(val);
    }
    inline bool DocGet(const char* szID, uint64_t& val) {
        val = 0;
        return DocGetNum64(szID, &val) == sizeof(val);
    }
    inline bool DocGetBlobEx(const char* szID, void* p, uint32_t n)
    {
        if (DocGetBlob(szID, p, n) == n)
            return true;

        Memset(p, 0, n);
        return false;
    }
    inline bool DocGet(const char* szID, ContractID& val) {
        return DocGetBlobEx(szID, &val, sizeof(val));
    }
    inline bool DocGet(const char* szID, PubKey& val) {
        return DocGetBlobEx(szID, &val, sizeof(val));
    }


} // namespace Env

namespace Utils {

    template <typename T>
    inline void ZeroObject(T& x) {
        Env::Memset(&x, 0, sizeof(x));
    }

} // namespace Utils

namespace std {
    
    template <typename T>
    T&& move(T& x) {
        return (T&&) x;
    }

    template <typename T>
    void swap(T& a, T& b) {
        T tmp(move(a));
        a = move(b);
        b = move(tmp);
    }
}
