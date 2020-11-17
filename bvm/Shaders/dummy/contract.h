#pragma once

namespace Dummy
{
#pragma pack (push, 1)

    struct MathTest1
    {
        static const uint32_t s_iMethod = 3;
        uint64_t m_Value; // retval
        uint64_t m_Rate; // point is after 32 bits
        uint64_t m_Factor; // point is after 32 bits

        uint64_t m_Try;
        uint8_t m_IsOk; // m_Try <(?)= m_Value * m_Rate * m_Factor / 2^64

        template <bool bToShader>
        void Convert()
        {
            ConvertOrd<bToShader>(m_Value);
            ConvertOrd<bToShader>(m_Rate);
            ConvertOrd<bToShader>(m_Factor);
            ConvertOrd<bToShader>(m_Try);
            ConvertOrd<bToShader>(m_IsOk);
        }
    };

    struct DivTest1
    {
        static const uint32_t s_iMethod = 4;
        uint64_t m_Nom;
        uint64_t m_Denom;

        template <bool bToShader>
        void Convert()
        {
            ConvertOrd<bToShader>(m_Nom);
            ConvertOrd<bToShader>(m_Denom);
        }
    };

    struct InfCycle
    {
        static const uint32_t s_iMethod = 5;

        uint32_t m_Val;

        template <bool bToShader>
        void Convert()
        {
            ConvertOrd<bToShader>(m_Val);
        }
    };

    struct Hash1
    {
        static const uint32_t s_iMethod = 6;

        uint8_t m_pInp[10];
        uint8_t m_pRes[32];

        template <bool bToShader>
        void Convert()
        {
        }
    };

#pragma pack (pop)

}
