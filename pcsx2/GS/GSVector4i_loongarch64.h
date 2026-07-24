// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0

#pragma once

#include "common/Assertions.h"

class alignas(16) GSVector4i
{
    static const GSVector4i m_xff[17];
    static const GSVector4i m_x0f[17];

    struct cxpr_init_tag
    {
    };
    static constexpr cxpr_init_tag cxpr_init{};

    constexpr GSVector4i(cxpr_init_tag, int x, int y, int z, int w)
    : I32{x, y, z, w}
    {
    }

    constexpr GSVector4i(cxpr_init_tag, short s0, short s1, short s2, short s3, short s4, short s5, short s6, short s7)
    : I16{s0, s1, s2, s3, s4, s5, s6, s7}
    {
    }

    constexpr GSVector4i(cxpr_init_tag, char b0, char b1, char b2, char b3, char b4, char b5, char b6, char b7, char b8, char b9, char b10, char b11, char b12, char b13, char b14, char b15)
    #if !defined(__APPLE__) && !defined(_MSC_VER) && !defined(ARCH_LOONGARCH64)
    : U8{b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15}
    #else
    : I8{b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15}
    #endif
    {
    }

public:
    union
    {
        struct
        {
            int x, y, z, w;
        };
        struct
        {
            int r, g, b, a;
        };
        struct
        {
            int left, top, right, bottom;
        };
        int v[4];
        float F32[4];
        s8 I8[16];
        s16 I16[8];
        s32 I32[4];
        s64 I64[2];
        u8 U8[16];
        u16 U16[8];
        u32 U32[4];
        u64 U64[2];
        v4i32 v4s;
    };

    GSVector4i() = default;

    constexpr static GSVector4i cxpr(int x, int y, int z, int w)
    {
        return GSVector4i(cxpr_init, x, y, z, w);
    }

    constexpr static GSVector4i cxpr(int x)
    {
        return GSVector4i(cxpr_init, x, x, x, x);
    }

    constexpr static GSVector4i cxpr16(short s0, short s1, short s2, short s3, short s4, short s5, short s6, short s7)
    {
        return GSVector4i(cxpr_init, s0, s1, s2, s3, s4, s5, s6, s7);
    }

    constexpr static GSVector4i cxpr8(char b0, char b1, char b2, char b3, char b4, char b5, char b6, char b7, char b8, char b9, char b10, char b11, char b12, char b13, char b14, char b15)
    {
        return GSVector4i(cxpr_init, b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15);
    }

    __forceinline GSVector4i(int x, int y, int z, int w)
    {
        GSVector4i xz = load(x).upl32(load(z));
        GSVector4i yw = load(y).upl32(load(w));

        *this = xz.upl32(yw);
    }

    __forceinline GSVector4i(int x, int y)
    {
        *this = load(x).upl32(load(y));
    }

    __forceinline GSVector4i(short s0, short s1, short s2, short s3, short s4, short s5, short s6, short s7)
    : I16{s0, s1, s2, s3, s4, s5, s6, s7}
    {
    }

    constexpr GSVector4i(char b0, char b1, char b2, char b3, char b4, char b5, char b6, char b7, char b8, char b9, char b10, char b11, char b12, char b13, char b14, char b15)
    #if !defined(__APPLE__) && !defined(_MSC_VER) && !defined(ARCH_LOONGARCH64)
    : U8{b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15}
    #else
    : I8{b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15}
    #endif
    {
    }

    __forceinline explicit GSVector4i(const GSVector2i& v)
    {
        v4s = __lsx_vldi(0);
        v4s = __lsx_vinsgr2vr_w(v4s, v.v[0], 0);
        v4s = __lsx_vinsgr2vr_w(v4s, v.v[1], 1);
    }

    // MSVC has bad codegen for the constexpr version when applied to non-constexpr things (https://godbolt.org/z/h8qbn7), so leave the non-constexpr version default
    __forceinline explicit GSVector4i(int i)
    {
        *this = i;
    }

    __forceinline constexpr explicit GSVector4i(v4i32 m)
    : v4s(m)
    {
    }

    __forceinline explicit GSVector4i(const GSVector4& v, bool truncate = true);

    __forceinline static GSVector4i cast(const GSVector4& v);

    __forceinline void operator=(int i)
    {
        v4s = __lsx_vreplgr2vr_w(i);
    }

    __forceinline operator v4i32() const
    {
        return v4s;
    }

    // rect

    __forceinline int width() const
    {
        return right - left;
    }

    __forceinline int height() const
    {
        return bottom - top;
    }

    __forceinline GSVector4i rsize() const
    {
        return sub32(xyxy()); // same as GSVector4i(0, 0, width(), height());
    }

    __forceinline unsigned int rarea() const
    {
        return width() * height();
    }

    __forceinline bool rempty() const
    {
        v4i32 v = lt32(zwzw());
        v4i32 t = __lsx_vmin_w(v, __lsx_vilvl_w(v, v));

        return __lsx_vpickve2gr_w(t, 0) == 0;
    }

    __forceinline GSVector4i runion(const GSVector4i& a) const
    {
        return min_i32(a).upl64(max_i32(a).srl<8>());
    }

    __forceinline GSVector4i rintersect(const GSVector4i& a) const
    {
        return sat_i32(a);
    }

    __forceinline bool rintersects(const GSVector4i& v) const
    {
        return !rintersect(v).rempty();
    }

    __forceinline bool rcontains(const GSVector4i& v) const
    {
        return rintersect(v).eq(v);
    }

    template <Align_Mode mode>
    GSVector4i _ralign_helper(const GSVector4i& mask) const
    {
        GSVector4i v;

        switch (mode)
        {
            case Align_Inside:
                v = *this + mask;
                break;
            case Align_Outside:
                v = *this + mask.zwxy();
                break;
            case Align_NegInf:
                v = *this;
                break;
            case Align_PosInf:
                v = *this + mask.xyxy();
                break;
            default:
                pxAssert(0);
                break;
        }

        return v.andnot(mask.xyxy());
    }

    /// Align the rect using mask values that already have one subtracted (1 << n - 1 aligns to 1 << n)
    template <Align_Mode mode>
    GSVector4i ralign_presub(const GSVector2i& a) const
    {
        return _ralign_helper<mode>(GSVector4i(a));
    }

    template <Align_Mode mode>
    GSVector4i ralign(const GSVector2i& a) const
    {
        // a must be 1 << n

        return _ralign_helper<mode>(GSVector4i(a) - GSVector4i(1, 1));
    }

    GSVector4i fit(int arx, int ary) const;

    GSVector4i fit(int preset) const;

    //

    __forceinline u32 rgba32() const
    {
        GSVector4i v = *this;

        v = v.ps32(v);
        v = v.pu16(v);

        return (u32)store(v);
    }

    __forceinline GSVector4i sat_i8(const GSVector4i& a, const GSVector4i& b) const
    {
        return max_i8(a).min_i8(b);
    }

    __forceinline GSVector4i sat_i8(const GSVector4i& a) const
    {
        return max_i8(a.xyxy()).min_i8(a.zwzw());
    }

    __forceinline GSVector4i sat_i16(const GSVector4i& a, const GSVector4i& b) const
    {
        return max_i16(a).min_i16(b);
    }

    __forceinline GSVector4i sat_i16(const GSVector4i& a) const
    {
        return max_i16(a.xyxy()).min_i16(a.zwzw());
    }

    __forceinline GSVector4i sat_i32(const GSVector4i& a, const GSVector4i& b) const
    {
        return max_i32(a).min_i32(b);
    }

    __forceinline GSVector4i sat_i32(const GSVector4i& a) const
    {
        return max_i32(a.xyxy()).min_i32(a.zwzw());
    }

    __forceinline GSVector4i sat_u8(const GSVector4i& a, const GSVector4i& b) const
    {
        return max_u8(a).min_u8(b);
    }

    __forceinline GSVector4i sat_u8(const GSVector4i& a) const
    {
        return max_u8(a.xyxy()).min_u8(a.zwzw());
    }

    __forceinline GSVector4i sat_u16(const GSVector4i& a, const GSVector4i& b) const
    {
        return max_u16(a).min_u16(b);
    }

    __forceinline GSVector4i sat_u16(const GSVector4i& a) const
    {
        return max_u16(a.xyxy()).min_u16(a.zwzw());
    }

    __forceinline GSVector4i sat_u32(const GSVector4i& a, const GSVector4i& b) const
    {
        return max_u32(a).min_u32(b);
    }

    __forceinline GSVector4i sat_u32(const GSVector4i& a) const
    {
        return max_u32(a.xyxy()).min_u32(a.zwzw());
    }

    __forceinline GSVector4i min_i8(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vmin_b(v4s, a.v4s));
    }

    __forceinline GSVector4i max_i8(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vmax_b(v4s, a.v4s));
    }

    __forceinline GSVector4i min_i16(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vmin_h(v4s, a.v4s));
    }

    __forceinline GSVector4i max_i16(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vmax_h(v4s, a.v4s));
    }

    __forceinline GSVector4i min_i32(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vmin_w(v4s, a.v4s));
    }

    __forceinline GSVector4i max_i32(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vmax_w(v4s, a.v4s));
    }

    __forceinline GSVector4i min_u8(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vmin_bu(v4s, a.v4s));
    }

    __forceinline GSVector4i max_u8(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vmax_bu(v4s, a.v4s));
    }

    __forceinline GSVector4i min_u16(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vmin_hu(v4s, a.v4s));
    }

    __forceinline GSVector4i max_u16(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vmax_hu(v4s, a.v4s));
    }

    __forceinline GSVector4i min_u32(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vmin_wu(v4s, a.v4s));
    }

    __forceinline GSVector4i max_u32(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vmax_wu(v4s, a.v4s));
    }

    // Hopefully this code optimizes alright...
    __forceinline s32 minv_s32() const
    {
        return std::min(
            std::min(__lsx_vpickve2gr_w(v4s, 0), __lsx_vpickve2gr_w(v4s, 1)),
                        std::min(__lsx_vpickve2gr_w(v4s, 2), __lsx_vpickve2gr_w(v4s, 3)));
    }

    __forceinline u32 minv_u32() const
    {
        return std::min(
            std::min(__lsx_vpickve2gr_wu(v4s, 0), __lsx_vpickve2gr_wu(v4s, 1)),
                        std::min(__lsx_vpickve2gr_wu(v4s, 2), __lsx_vpickve2gr_wu(v4s, 3)));
    }

    __forceinline s32 maxv_s32() const
    {
        return std::max(
            std::max(__lsx_vpickve2gr_w(v4s, 0), __lsx_vpickve2gr_w(v4s, 1)),
                        std::max(__lsx_vpickve2gr_w(v4s, 2), __lsx_vpickve2gr_w(v4s, 3)));
    }

    __forceinline u32 maxv_u32() const
    {
        return std::max(
            std::max(__lsx_vpickve2gr_wu(v4s, 0), __lsx_vpickve2gr_wu(v4s, 1)),
                        std::max(__lsx_vpickve2gr_wu(v4s, 2), __lsx_vpickve2gr_wu(v4s, 3)));
    }

    __forceinline static int min_i16(int a, int b)
    {
        return store(load(a).min_i16(load(b)));
    }

    __forceinline GSVector4i clamp8() const
    {
        return pu16().upl8();
    }

    __forceinline GSVector4i blend8(const GSVector4i& a, const GSVector4i& mask) const
    {
        v4i32 mask2 = __lsx_vsrai_b(mask.v4s, 7);
        return GSVector4i(__lsx_vbitsel_v(v4s, a.v4s, mask2));
    }

    template <int mask>
    __forceinline GSVector4i blend16(const GSVector4i& a) const
    {
        const uint16_t _mask[8] = {((mask) & (1 << 0)) ? (uint16_t)-1 : 0x0,
            ((mask) & (1 << 1)) ? (uint16_t)-1 : 0x0,
            ((mask) & (1 << 2)) ? (uint16_t)-1 : 0x0,
            ((mask) & (1 << 3)) ? (uint16_t)-1 : 0x0,
            ((mask) & (1 << 4)) ? (uint16_t)-1 : 0x0,
            ((mask) & (1 << 5)) ? (uint16_t)-1 : 0x0,
            ((mask) & (1 << 6)) ? (uint16_t)-1 : 0x0,
            ((mask) & (1 << 7)) ? (uint16_t)-1 : 0x0};
            return GSVector4i(__lsx_vbitsel_v(v4s, a.v4s, __lsx_vld(_mask, 0)));
    }

    template <int mask>
    __forceinline GSVector4i blend32(const GSVector4i& v) const
    {
        constexpr int bit3 = ((mask & 8) * 3) << 3;
        constexpr int bit2 = ((mask & 4) * 3) << 2;
        constexpr int bit1 = ((mask & 2) * 3) << 1;
        constexpr int bit0 = (mask & 1) * 3;
        return blend16<bit3 | bit2 | bit1 | bit0>(v);
    }

    /// Equivalent to blend with the given mask broadcasted across the vector
    /// May be faster than blend in some cases
    template <u32 mask>
    __forceinline GSVector4i smartblend(const GSVector4i& a) const
    {
        if (mask == 0)
            return *this;
        if (mask == 0xffffffff)
            return a;

        if (mask == 0x0000ffff)
            return blend16<0x55>(a);
        if (mask == 0xffff0000)
            return blend16<0xaa>(a);

        for (int i = 0; i < 32; i += 8)
        {
            u8 byte = (mask >> i) & 0xff;
            if (byte != 0xff && byte != 0)
                return blend(a, GSVector4i(mask));
        }

        return blend8(a, GSVector4i(mask));
    }

    __forceinline GSVector4i blend(const GSVector4i& a, const GSVector4i& mask) const
    {
        return GSVector4i(__lsx_vbitsel_v(v4s, a.v4s, mask.v4s));
    }

    __forceinline GSVector4i mix16(const GSVector4i& a) const
    {
        return blend16<0xaa>(a);
    }

    __forceinline GSVector4i shuffle8(const GSVector4i& mask) const
    {
        const v4i32 shuffled = __lsx_vshuf_b(v4s, v4s, mask.v4s);
        const v4i32 zero = __lsx_vldi(0);
        const v4i32 high_bit_set = __lsx_vslt_b(mask.v4s, zero);
        return GSVector4i(__lsx_vbitsel_v(shuffled, zero, high_bit_set));
    }

    __forceinline GSVector4i ps16(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vssrarni_b_h(a.v4s, v4s, 0));
    }

    __forceinline GSVector4i ps16() const
    {
        return GSVector4i(__lsx_vssrarni_b_h(v4s, v4s, 0));
    }

    __forceinline GSVector4i pu16(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vssrarni_bu_h(a.v4s, v4s, 0));
    }

    __forceinline GSVector4i pu16() const
    {
        return GSVector4i(__lsx_vssrarni_bu_h(v4s, v4s, 0));
    }

    __forceinline GSVector4i ps32(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vssrarni_h_w(a.v4s, v4s, 0));
    }

    __forceinline GSVector4i ps32() const
    {
        return GSVector4i(__lsx_vssrarni_h_w(v4s, v4s, 0));
    }

    __forceinline GSVector4i pu32(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vssrarni_hu_w(a.v4s, v4s, 0));
    }

    __forceinline GSVector4i pu32() const
    {
        return GSVector4i(__lsx_vssrarni_hu_w(v4s, v4s, 0));
    }

    __forceinline GSVector4i pkl32(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vilvl_w(a.v4s, v4s));
    }

    __forceinline GSVector4i pku32(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vilvh_w(a.v4s, v4s));
    }

    __forceinline GSVector4i upl8(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vilvl_b(a.v4s, v4s));
    }

    __forceinline GSVector4i uph8(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vilvh_b(a.v4s, v4s));
    }

    __forceinline GSVector4i upl16(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vilvl_h(a.v4s, v4s));
    }

    __forceinline GSVector4i uph16(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vilvh_h(a.v4s, v4s));
    }

    __forceinline GSVector4i upl32(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vilvl_w(a.v4s, v4s));
    }

    __forceinline GSVector4i uph32(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vilvh_w(a.v4s, v4s));
    }

    __forceinline GSVector4i upl64(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vilvl_d(a.v4s, v4s));
    }

    __forceinline GSVector4i uph64(const GSVector4i& a) const
    {
        return GSVector4i(__lsx_vilvh_d(a.v4s, v4s));
    }

    __forceinline GSVector4i upl8() const
    {
        return GSVector4i(__lsx_vilvl_b(__lsx_vldi(0), v4s));
    }

    __forceinline GSVector4i uph8() const
    {
        return GSVector4i(__lsx_vilvh_b(__lsx_vldi(0), v4s));
    }

    __forceinline GSVector4i upl16() const
    {
        return GSVector4i(__lsx_vilvl_h(__lsx_vldi(0), v4s));
    }

    __forceinline GSVector4i uph16() const
    {
        return GSVector4i(__lsx_vilvh_h(__lsx_vldi(0), v4s));
    }

    __forceinline GSVector4i upl32() const
    {
        return GSVector4i(__lsx_vilvl_w(__lsx_vldi(0), v4s));
    }

    __forceinline GSVector4i uph32() const
    {
        return GSVector4i(__lsx_vilvh_w(__lsx_vldi(0), v4s));
    }

    __forceinline GSVector4i upl64() const
    {
        return GSVector4i(__lsx_vilvl_d(__lsx_vldi(0), v4s));
    }

    __forceinline GSVector4i uph64() const
    {
        return GSVector4i(__lsx_vilvh_d(__lsx_vldi(0), v4s));
    }

    __forceinline GSVector4i i8to16() const
    {
        return GSVector4i(__lsx_vilvl_b(__lsx_vsrai_b(v4s, 7), v4s));
    }

    __forceinline GSVector4i u8to16() const
    {
        return GSVector4i(__lsx_vilvl_b(__lsx_vldi(0), v4s));
    }

    __forceinline GSVector4i i8to32() const
    {
        v4i32 v = __lsx_vilvl_b(__lsx_vsrai_b(v4s, 7), v4s);
        return GSVector4i(__lsx_vilvl_h(__lsx_vsrai_h(v, 15), v));
    }

    __forceinline GSVector4i u8to32() const
    {
        return GSVector4i(__lsx_vilvl_h(__lsx_vldi(0), __lsx_vilvl_b(__lsx_vldi(0), v4s)));
    }

    __forceinline GSVector4i i8to64() const
    {
        v4i32 v = i8to32();
        return GSVector4i(__lsx_vilvl_w(__lsx_vsrai_w(v, 31), v));
    }

    __forceinline GSVector4i u8to64() const
    {
        return GSVector4i(__lsx_vilvl_w(__lsx_vldi(0), u8to32()));
    }

    __forceinline GSVector4i i16to32() const
    {
        return GSVector4i(__lsx_vilvl_h(__lsx_vsrai_h(v4s, 15), v4s));
    }

    __forceinline GSVector4i u16to32() const
    {
        return GSVector4i(__lsx_vilvl_h(__lsx_vldi(0), v4s));
    }

    __forceinline GSVector4i i16to64() const
    {
        v4i32 v = i16to32();
        return GSVector4i(__lsx_vilvl_w(__lsx_vsrai_w(v, 31), v));
    }

    __forceinline GSVector4i u16to64() const
    {
        return GSVector4i(__lsx_vilvl_w(__lsx_vldi(0), u16to32()));
    }

    __forceinline GSVector4i i32to64() const
    {
        return GSVector4i(__lsx_vilvl_w(__lsx_vsrai_w(v4s, 31), v4s));
    }

    __forceinline GSVector4i u32to64() const
    {
        return GSVector4i(__lsx_vilvl_w(__lsx_vldi(0), v4s));
    }

    template <int i>
    __forceinline GSVector4i srl() const
    {
        return GSVector4i(__lsx_vbsrl_v(v4s, i));
    }

    template <int i>
    __forceinline GSVector4i srl(const GSVector4i& v)
    {
        if constexpr (i >= 16)
            return GSVector4i(__lsx_vbsrl_v(v.v4s, i - 16));
        else
            return GSVector4i(__lsx_vor_v(__lsx_vbsrl_v(v4s, i), __lsx_vbsll_v(v.v4s, 16 - i)));
    }

    template <int i>
    __forceinline GSVector4i sll() const
    {
        return GSVector4i(__lsx_vbsll_v(v4s, i));
    }

    template <int i>
    __forceinline GSVector4i sll16() const
    {
        return GSVector4i(__lsx_vslli_h(v4s, i));
    }

    __forceinline GSVector4i sll16(s32 i) const
    {
        return GSVector4i(__lsx_vsll_h(v4s, __lsx_vreplgr2vr_h(i)));
    }

    __forceinline GSVector4i sllv16(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsll_h(v4s, v.v4s));
    }

    template <int i>
    __forceinline GSVector4i srl16() const
    {
        return GSVector4i(__lsx_vsrli_h(v4s, i));
    }

    __forceinline GSVector4i srl16(s32 i) const
    {
        return GSVector4i(__lsx_vsrl_h(v4s, __lsx_vreplgr2vr_h(i)));
    }

    __forceinline GSVector4i srlv16(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsrl_h(v4s, v.v4s));
    }

    template <int i>
    __forceinline GSVector4i sra16() const
    {
        constexpr int count = (i & ~15) ? 15 : i;
        return GSVector4i(__lsx_vsrai_h(v4s, count));
    }

    __forceinline GSVector4i sra16(s32 i) const
    {
        return GSVector4i(__lsx_vsra_h(v4s, __lsx_vreplgr2vr_h(i)));
    }

    __forceinline GSVector4i srav16(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsra_h(v4s, v.v4s));
    }

    template <int i>
    __forceinline GSVector4i sll32() const
    {
        return GSVector4i(__lsx_vslli_w(v4s, i));
    }

    __forceinline GSVector4i sll32(s32 i) const { return GSVector4i(__lsx_vsll_w(v4s, __lsx_vreplgr2vr_w(i))); }

    __forceinline GSVector4i sllv32(const GSVector4i& v) const { return GSVector4i(__lsx_vsll_w(v4s, v.v4s)); }

    template <int i>
    __forceinline GSVector4i srl32() const
    {
        return GSVector4i(__lsx_vsrli_w(v4s, i));
    }

    __forceinline GSVector4i srl32(s32 i) const
    {
        return GSVector4i(__lsx_vsrl_w(v4s, __lsx_vreplgr2vr_w(i)));
    }

    __forceinline GSVector4i srlv32(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsrl_w(v4s, v.v4s));
    }

    template <int i>
    __forceinline GSVector4i sra32() const
    {
        return GSVector4i(__lsx_vsrai_w(v4s, i));
    }

    __forceinline GSVector4i sra32(s32 i) const { return GSVector4i(__lsx_vsra_w(v4s, __lsx_vreplgr2vr_w(i))); }

    __forceinline GSVector4i srav32(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsra_w(v4s, v.v4s));
    }

    template <int i>
    __forceinline GSVector4i sll64() const
    {
        return GSVector4i(__lsx_vslli_d(v4s, i));
    }

    __forceinline GSVector4i sll64(s32 i) const
    {
        return GSVector4i(__lsx_vsll_d(v4s, __lsx_vreplgr2vr_d(i)));
    }

    __forceinline GSVector4i sllv64(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsll_d(v4s, v.v4s));
    }

    template <int i>
    __forceinline GSVector4i sra64() const
    {
        return GSVector4i(__lsx_vsrai_d(v4s, i));
    }

    __forceinline GSVector4i sra64(s32 i) const
    {
        return GSVector4i(__lsx_vsra_d(v4s, __lsx_vreplgr2vr_d(i)));
    }

    __forceinline GSVector4i srav64(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsra_d(v4s, v.v4s));
    }

    template <int i>
    __forceinline GSVector4i srl64() const
    {
        return GSVector4i(__lsx_vsrli_d(v4s, i));
    }

    __forceinline GSVector4i srl64(s32 i) const
    {
        return GSVector4i(__lsx_vsrl_d(v4s, __lsx_vreplgr2vr_d(i)));
    }

    __forceinline GSVector4i srlv64(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsrl_d(v4s, v.v4s));
    }

    __forceinline GSVector4i abs32() const
    {
        return GSVector4i(__lsx_vsigncov_w(v4s, v4s));
    }

    __forceinline GSVector4i add8(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vadd_b(v4s, v.v4s));
    }

    __forceinline GSVector4i add16(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vadd_h(v4s, v.v4s));
    }

    __forceinline GSVector4i add32(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vadd_w(v4s, v.v4s));
    }

    __forceinline GSVector4i hadd32(const GSVector4i& v) const
    {
        return GSVector4i(__builtin_shufflevector(v4s, v.v4s, 0, 2, 4, 6) + __builtin_shufflevector(v4s, v.v4s, 1, 3, 5, 7));
    }

    __forceinline GSVector4i adds8(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsadd_b(v4s, v.v4s));
    }

    __forceinline GSVector4i adds16(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsadd_h(v4s, v.v4s));
    }

    __forceinline GSVector4i hadds16(const GSVector4i& v) const
    {
        v4i32 a = (v4i32)__builtin_shufflevector((v8i16)v4s, (v8i16)v.v4s, 0, 2, 4, 6, 8, 10, 12, 14);
        v4i32 b = (v4i32)__builtin_shufflevector((v8i16)v4s, (v8i16)v.v4s, 1, 3, 5, 7, 9, 11, 13, 15);
        return GSVector4i(__lsx_vsadd_h(a, b));
    }

    __forceinline GSVector4i addus8(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsadd_bu(v4s, v.v4s));
    }

    __forceinline GSVector4i addus16(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsadd_hu(v4s, v.v4s));
    }

    __forceinline GSVector4i sub8(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsub_b(v4s, v.v4s));
    }

    __forceinline GSVector4i sub16(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsub_h(v4s, v.v4s));
    }

    __forceinline GSVector4i sub32(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsub_w(v4s, v.v4s));
    }

    __forceinline GSVector4i hsub32(const GSVector4i& v) const
    {
        return GSVector4i(__builtin_shufflevector(v4s, v.v4s, 0, 2, 4, 6) - __builtin_shufflevector(v4s, v.v4s, 1, 3, 5, 7));
    }

    __forceinline GSVector4i subs8(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vssub_b(v4s, v.v4s));
    }

    __forceinline GSVector4i subs16(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vssub_h(v4s, v.v4s));
    }

    __forceinline GSVector4i subus8(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vssub_bu(v4s, v.v4s));
    }

    __forceinline GSVector4i subus16(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vssub_hu(v4s, v.v4s));
    }

    __forceinline GSVector4i avg8(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vavgr_bu(v4s, v.v4s));
    }

    __forceinline GSVector4i avg16(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vavgr_hu(v4s, v.v4s));
    }

    __forceinline GSVector4i mul16hs(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vmuh_h(v4s, v.v4s));
    }

    __forceinline GSVector4i mul16l(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vmul_h(v4s, v.v4s));
    }

    __forceinline GSVector4i mul16hrs(const GSVector4i& v) const
    {
        v4i32 mul_lo = __lsx_vmulwev_w_h(v4s, v.v4s);
        v4i32 mul_hi = __lsx_vmulwod_w_h(v4s, v.v4s);
        const v8i16 packed = (v8i16)__lsx_vssrarni_h_w(mul_hi, mul_lo, 15);
        return GSVector4i((v4i32)__builtin_shufflevector(packed, packed, 0, 4, 1, 5, 2, 6, 3, 7));
    }

    template <int shift>
    __forceinline GSVector4i lerp16(const GSVector4i& a, const GSVector4i& f) const
    {
        // (a - this) * f << shift + this

        return add16(a.sub16(*this).modulate16<shift>(f));
    }

    template <int shift>
    __forceinline static GSVector4i lerp16(const GSVector4i& a, const GSVector4i& b, const GSVector4i& c)
    {
        // (a - b) * c << shift

        return a.sub16(b).modulate16<shift>(c);
    }

    template <int shift>
    __forceinline static GSVector4i lerp16(const GSVector4i& a, const GSVector4i& b, const GSVector4i& c, const GSVector4i& d)
    {
        // (a - b) * c << shift + d

        return d.add16(a.sub16(b).modulate16<shift>(c));
    }

    __forceinline GSVector4i lerp16_4(const GSVector4i& a, const GSVector4i& f) const
    {
        // (a - this) * f >> 4 + this (a, this: 8-bit, f: 4-bit)

        return add16(a.sub16(*this).mul16l(f).sra16<4>());
    }

    template <int shift>
    __forceinline GSVector4i modulate16(const GSVector4i& f) const
    {
        // a * f << shift

        return sll16<shift + 1>().mul16hs(f);
    }

    __forceinline bool eq(const GSVector4i& v) const
    {
        return __lsx_bz_v(__lsx_vxor_v(v4s, v.v4s));
    }

    __forceinline GSVector4i eq8(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vseq_b(v4s, v.v4s));
    }

    __forceinline GSVector4i eq16(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vseq_h(v4s, v.v4s));
    }

    __forceinline GSVector4i eq32(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vseq_w(v4s, v.v4s));
    }

    __forceinline GSVector4i eq64(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vseq_d(v4s, v.v4s));
    }

    __forceinline GSVector4i neq8(const GSVector4i& v) const
    {
        return ~eq8(v);
    }

    __forceinline GSVector4i neq16(const GSVector4i& v) const
    {
        return ~eq16(v);
    }

    __forceinline GSVector4i neq32(const GSVector4i& v) const
    {
        return ~eq32(v);
    }

    __forceinline GSVector4i gt8(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vslt_b(v.v4s, v4s));
    }

    __forceinline GSVector4i gt16(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vslt_h(v.v4s, v4s));
    }

    __forceinline GSVector4i gt32(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vslt_w(v.v4s, v4s));
    }

    __forceinline GSVector4i ge8(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsle_b(v.v4s, v4s));
    }

    __forceinline GSVector4i ge16(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsle_h(v.v4s, v4s));
    }

    __forceinline GSVector4i ge32(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsle_w(v.v4s, v4s));
    }

    __forceinline GSVector4i lt8(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vslt_b(v4s, v.v4s));
    }

    __forceinline GSVector4i lt16(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vslt_h(v4s, v.v4s));
    }

    __forceinline GSVector4i lt32(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vslt_w(v4s, v.v4s));
    }

    __forceinline GSVector4i le8(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsle_b(v4s, v.v4s));
    }

    __forceinline GSVector4i le16(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsle_h(v4s, v.v4s));
    }

    __forceinline GSVector4i le32(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vsle_w(v4s, v.v4s));
    }


    __forceinline GSVector4i andnot(const GSVector4i& v) const
    {
        return GSVector4i(__lsx_vandn_v(v.v4s, v4s));
    }

    __forceinline int mask() const
    {
        return __lsx_vpickve2gr_w(__lsx_vmskltz_b(v4s), 0);
    }

    __forceinline bool alltrue() const
    {
        return mask() == 0xffff;
    }

    __forceinline bool allfalse() const
    {
        return mask() == 0;
    }

    template <int i>
    __forceinline GSVector4i insert8(int a) const
    {
        return GSVector4i(__lsx_vinsgr2vr_b(v4s, a, i));
    }

    template <int i>
    __forceinline int extract8() const
    {
        return __lsx_vpickve2gr_bu(v4s, i);
    }

    template <int i>
    __forceinline GSVector4i insert16(int a) const
    {
        return GSVector4i(__lsx_vinsgr2vr_h(v4s, a, i));
    }

    template <int i>
    __forceinline int extract16() const
    {
        return __lsx_vpickve2gr_hu(v4s, i);
    }

    template <int i>
    __forceinline GSVector4i insert32(int a) const
    {
        return GSVector4i(__lsx_vinsgr2vr_w(v4s, a, i));
    }

    template <int i>
    __forceinline int extract32() const
    {
        return __lsx_vpickve2gr_w(v4s, i);
    }

    template <int i>
    __forceinline GSVector4i insert64(s64 a) const
    {
        return GSVector4i(__lsx_vinsgr2vr_d(v4s, a, i));
    }

    template <int i>
    __forceinline s64 extract64() const
    {
        return __lsx_vpickve2gr_d(v4s, i);
    }

    template <int src, class T>
    __forceinline GSVector4i gather8_4(const T* ptr) const
    {
        GSVector4i v;

        v = load((int)ptr[extract8<src + 0>() & 0xf]);
        v = v.insert8<1>((int)ptr[extract8<src + 0>() >> 4]);
        v = v.insert8<2>((int)ptr[extract8<src + 1>() & 0xf]);
        v = v.insert8<3>((int)ptr[extract8<src + 1>() >> 4]);
        v = v.insert8<4>((int)ptr[extract8<src + 2>() & 0xf]);
        v = v.insert8<5>((int)ptr[extract8<src + 2>() >> 4]);
        v = v.insert8<6>((int)ptr[extract8<src + 3>() & 0xf]);
        v = v.insert8<7>((int)ptr[extract8<src + 3>() >> 4]);
        v = v.insert8<8>((int)ptr[extract8<src + 4>() & 0xf]);
        v = v.insert8<9>((int)ptr[extract8<src + 4>() >> 4]);
        v = v.insert8<10>((int)ptr[extract8<src + 5>() & 0xf]);
        v = v.insert8<11>((int)ptr[extract8<src + 5>() >> 4]);
        v = v.insert8<12>((int)ptr[extract8<src + 6>() & 0xf]);
        v = v.insert8<13>((int)ptr[extract8<src + 6>() >> 4]);
        v = v.insert8<14>((int)ptr[extract8<src + 7>() & 0xf]);
        v = v.insert8<15>((int)ptr[extract8<src + 7>() >> 4]);

        return v;
    }

    template <class T>
    __forceinline GSVector4i gather8_8(const T* ptr) const
    {
        GSVector4i v;

        v = load((int)ptr[extract8<0>()]);
        v = v.insert8<1>((int)ptr[extract8<1>()]);
        v = v.insert8<2>((int)ptr[extract8<2>()]);
        v = v.insert8<3>((int)ptr[extract8<3>()]);
        v = v.insert8<4>((int)ptr[extract8<4>()]);
        v = v.insert8<5>((int)ptr[extract8<5>()]);
        v = v.insert8<6>((int)ptr[extract8<6>()]);
        v = v.insert8<7>((int)ptr[extract8<7>()]);
        v = v.insert8<8>((int)ptr[extract8<8>()]);
        v = v.insert8<9>((int)ptr[extract8<9>()]);
        v = v.insert8<10>((int)ptr[extract8<10>()]);
        v = v.insert8<11>((int)ptr[extract8<11>()]);
        v = v.insert8<12>((int)ptr[extract8<12>()]);
        v = v.insert8<13>((int)ptr[extract8<13>()]);
        v = v.insert8<14>((int)ptr[extract8<14>()]);
        v = v.insert8<15>((int)ptr[extract8<15>()]);

        return v;
    }

    template <int dst, class T>
    __forceinline GSVector4i gather8_16(const T* ptr, const GSVector4i& a) const
    {
        GSVector4i v = a;

        v = v.insert8<dst + 0>((int)ptr[extract16<0>()]);
        v = v.insert8<dst + 1>((int)ptr[extract16<1>()]);
        v = v.insert8<dst + 2>((int)ptr[extract16<2>()]);
        v = v.insert8<dst + 3>((int)ptr[extract16<3>()]);
        v = v.insert8<dst + 4>((int)ptr[extract16<4>()]);
        v = v.insert8<dst + 5>((int)ptr[extract16<5>()]);
        v = v.insert8<dst + 6>((int)ptr[extract16<6>()]);
        v = v.insert8<dst + 7>((int)ptr[extract16<7>()]);

        return v;
    }

    template <int dst, class T>
    __forceinline GSVector4i gather8_32(const T* ptr, const GSVector4i& a) const
    {
        GSVector4i v = a;

        v = v.insert8<dst + 0>((int)ptr[extract32<0>()]);
        v = v.insert8<dst + 1>((int)ptr[extract32<1>()]);
        v = v.insert8<dst + 2>((int)ptr[extract32<2>()]);
        v = v.insert8<dst + 3>((int)ptr[extract32<3>()]);

        return v;
    }

    template <int src, class T>
    __forceinline GSVector4i gather16_4(const T* ptr) const
    {
        GSVector4i v;

        v = load((int)ptr[extract8<src + 0>() & 0xf]);
        v = v.insert16<1>((int)ptr[extract8<src + 0>() >> 4]);
        v = v.insert16<2>((int)ptr[extract8<src + 1>() & 0xf]);
        v = v.insert16<3>((int)ptr[extract8<src + 1>() >> 4]);
        v = v.insert16<4>((int)ptr[extract8<src + 2>() & 0xf]);
        v = v.insert16<5>((int)ptr[extract8<src + 2>() >> 4]);
        v = v.insert16<6>((int)ptr[extract8<src + 3>() & 0xf]);
        v = v.insert16<7>((int)ptr[extract8<src + 3>() >> 4]);

        return v;
    }

    template <int src, class T>
    __forceinline GSVector4i gather16_8(const T* ptr) const
    {
        GSVector4i v;

        v = load((int)ptr[extract8<src + 0>()]);
        v = v.insert16<1>((int)ptr[extract8<src + 1>()]);
        v = v.insert16<2>((int)ptr[extract8<src + 2>()]);
        v = v.insert16<3>((int)ptr[extract8<src + 3>()]);
        v = v.insert16<4>((int)ptr[extract8<src + 4>()]);
        v = v.insert16<5>((int)ptr[extract8<src + 5>()]);
        v = v.insert16<6>((int)ptr[extract8<src + 6>()]);
        v = v.insert16<7>((int)ptr[extract8<src + 7>()]);

        return v;
    }

    template <class T>
    __forceinline GSVector4i gather16_16(const T* ptr) const
    {
        GSVector4i v;

        v = load((int)ptr[extract16<0>()]);
        v = v.insert16<1>((int)ptr[extract16<1>()]);
        v = v.insert16<2>((int)ptr[extract16<2>()]);
        v = v.insert16<3>((int)ptr[extract16<3>()]);
        v = v.insert16<4>((int)ptr[extract16<4>()]);
        v = v.insert16<5>((int)ptr[extract16<5>()]);
        v = v.insert16<6>((int)ptr[extract16<6>()]);
        v = v.insert16<7>((int)ptr[extract16<7>()]);

        return v;
    }

    template <class T1, class T2>
    __forceinline GSVector4i gather16_16(const T1* ptr1, const T2* ptr2) const
    {
        GSVector4i v;

        v = load((int)ptr2[ptr1[extract16<0>()]]);
        v = v.insert16<1>((int)ptr2[ptr1[extract16<1>()]]);
        v = v.insert16<2>((int)ptr2[ptr1[extract16<2>()]]);
        v = v.insert16<3>((int)ptr2[ptr1[extract16<3>()]]);
        v = v.insert16<4>((int)ptr2[ptr1[extract16<4>()]]);
        v = v.insert16<5>((int)ptr2[ptr1[extract16<5>()]]);
        v = v.insert16<6>((int)ptr2[ptr1[extract16<6>()]]);
        v = v.insert16<7>((int)ptr2[ptr1[extract16<7>()]]);

        return v;
    }

    template <int dst, class T>
    __forceinline GSVector4i gather16_32(const T* ptr, const GSVector4i& a) const
    {
        GSVector4i v = a;

        v = v.insert16<dst + 0>((int)ptr[extract32<0>()]);
        v = v.insert16<dst + 1>((int)ptr[extract32<1>()]);
        v = v.insert16<dst + 2>((int)ptr[extract32<2>()]);
        v = v.insert16<dst + 3>((int)ptr[extract32<3>()]);

        return v;
    }

    template <int src, class T>
    __forceinline GSVector4i gather32_4(const T* ptr) const
    {
        GSVector4i v;

        v = load((int)ptr[extract8<src + 0>() & 0xf]);
        v = v.insert32<1>((int)ptr[extract8<src + 0>() >> 4]);
        v = v.insert32<2>((int)ptr[extract8<src + 1>() & 0xf]);
        v = v.insert32<3>((int)ptr[extract8<src + 1>() >> 4]);
        return v;
    }

    template <int src, class T>
    __forceinline GSVector4i gather32_8(const T* ptr) const
    {
        GSVector4i v;

        v = load((int)ptr[extract8<src + 0>()]);
        v = v.insert32<1>((int)ptr[extract8<src + 1>()]);
        v = v.insert32<2>((int)ptr[extract8<src + 2>()]);
        v = v.insert32<3>((int)ptr[extract8<src + 3>()]);

        return v;
    }

    template <int src, class T>
    __forceinline GSVector4i gather32_16(const T* ptr) const
    {
        GSVector4i v;

        v = load((int)ptr[extract16<src + 0>()]);
        v = v.insert32<1>((int)ptr[extract16<src + 1>()]);
        v = v.insert32<2>((int)ptr[extract16<src + 2>()]);
        v = v.insert32<3>((int)ptr[extract16<src + 3>()]);

        return v;
    }

    template <class T>
    __forceinline GSVector4i gather32_32(const T* ptr) const
    {
        GSVector4i v;

        v = load((int)ptr[extract32<0>()]);
        v = v.insert32<1>((int)ptr[extract32<1>()]);
        v = v.insert32<2>((int)ptr[extract32<2>()]);
        v = v.insert32<3>((int)ptr[extract32<3>()]);

        return v;
    }

    template <class T1, class T2>
    __forceinline GSVector4i gather32_32(const T1* ptr1, const T2* ptr2) const
    {
        GSVector4i v;

        v = load((int)ptr2[ptr1[extract32<0>()]]);
        v = v.insert32<1>((int)ptr2[ptr1[extract32<1>()]]);
        v = v.insert32<2>((int)ptr2[ptr1[extract32<2>()]]);
        v = v.insert32<3>((int)ptr2[ptr1[extract32<3>()]]);

        return v;
    }

    template <int src, class T>
    __forceinline GSVector4i gather64_4(const T* ptr) const
    {
        GSVector4i v;

        v = loadq((s64)ptr[extract8<src + 0>() & 0xf]);
        v = v.insert64<1>((s64)ptr[extract8<src + 0>() >> 4]);

        return v;
    }

    template <int src, class T>
    __forceinline GSVector4i gather64_8(const T* ptr) const
    {
        GSVector4i v;

        v = loadq((s64)ptr[extract8<src + 0>()]);
        v = v.insert64<1>((s64)ptr[extract8<src + 1>()]);

        return v;
    }

    template <int src, class T>
    __forceinline GSVector4i gather64_16(const T* ptr) const
    {
        GSVector4i v;

        v = loadq((s64)ptr[extract16<src + 0>()]);
        v = v.insert64<1>((s64)ptr[extract16<src + 1>()]);

        return v;
    }

    template <int src, class T>
    __forceinline GSVector4i gather64_32(const T* ptr) const
    {
        GSVector4i v;

        v = loadq((s64)ptr[extract32<src + 0>()]);
        v = v.insert64<1>((s64)ptr[extract32<src + 1>()]);

        return v;
    }

    template <class T>
    __forceinline GSVector4i gather64_64(const T* ptr) const
    {
        GSVector4i v;

        v = loadq((s64)ptr[extract64<0>()]);
        v = v.insert64<1>((s64)ptr[extract64<1>()]);

        return v;
    }

    template <class T>
    __forceinline void gather8_4(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
    {
        dst[0] = gather8_4<0>(ptr);
        dst[1] = gather8_4<8>(ptr);
    }

    __forceinline void gather8_8(const u8* RESTRICT ptr, GSVector4i* RESTRICT dst) const
    {
        dst[0] = gather8_8<>(ptr);
    }

    template <class T>
    __forceinline void gather16_4(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
    {
        dst[0] = gather16_4<0>(ptr);
        dst[1] = gather16_4<4>(ptr);
        dst[2] = gather16_4<8>(ptr);
        dst[3] = gather16_4<12>(ptr);
    }

    template <class T>
    __forceinline void gather16_8(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
    {
        dst[0] = gather16_8<0>(ptr);
        dst[1] = gather16_8<8>(ptr);
    }

    template <class T>
    __forceinline void gather16_16(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
    {
        dst[0] = gather16_16<>(ptr);
    }

    template <class T>
    __forceinline void gather32_4(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
    {
        dst[0] = gather32_4<0>(ptr);
        dst[1] = gather32_4<2>(ptr);
        dst[2] = gather32_4<4>(ptr);
        dst[3] = gather32_4<6>(ptr);
        dst[4] = gather32_4<8>(ptr);
        dst[5] = gather32_4<10>(ptr);
        dst[6] = gather32_4<12>(ptr);
        dst[7] = gather32_4<14>(ptr);
    }

    template <class T>
    __forceinline void gather32_8(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
    {
        dst[0] = gather32_8<0>(ptr);
        dst[1] = gather32_8<4>(ptr);
        dst[2] = gather32_8<8>(ptr);
        dst[3] = gather32_8<12>(ptr);
    }

    template <class T>
    __forceinline void gather32_16(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
    {
        dst[0] = gather32_16<0>(ptr);
        dst[1] = gather32_16<4>(ptr);
    }

    template <class T>
    __forceinline void gather32_32(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
    {
        dst[0] = gather32_32<>(ptr);
    }

    template <class T>
    __forceinline void gather64_4(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
    {
        dst[0] = gather64_4<0>(ptr);
        dst[1] = gather64_4<1>(ptr);
        dst[2] = gather64_4<2>(ptr);
        dst[3] = gather64_4<3>(ptr);
        dst[4] = gather64_4<4>(ptr);
        dst[5] = gather64_4<5>(ptr);
        dst[6] = gather64_4<6>(ptr);
        dst[7] = gather64_4<7>(ptr);
        dst[8] = gather64_4<8>(ptr);
        dst[9] = gather64_4<9>(ptr);
        dst[10] = gather64_4<10>(ptr);
        dst[11] = gather64_4<11>(ptr);
        dst[12] = gather64_4<12>(ptr);
        dst[13] = gather64_4<13>(ptr);
        dst[14] = gather64_4<14>(ptr);
        dst[15] = gather64_4<15>(ptr);
    }

    template <class T>
    __forceinline void gather64_8(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
    {
        dst[0] = gather64_8<0>(ptr);
        dst[1] = gather64_8<2>(ptr);
        dst[2] = gather64_8<4>(ptr);
        dst[3] = gather64_8<6>(ptr);
        dst[4] = gather64_8<8>(ptr);
        dst[5] = gather64_8<10>(ptr);
        dst[6] = gather64_8<12>(ptr);
        dst[7] = gather64_8<14>(ptr);
    }

    template <class T>
    __forceinline void gather64_16(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
    {
        dst[0] = gather64_16<0>(ptr);
        dst[1] = gather64_16<2>(ptr);
        dst[2] = gather64_16<4>(ptr);
        dst[3] = gather64_16<8>(ptr);
    }

    template <class T>
    __forceinline void gather64_32(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
    {
        dst[0] = gather64_32<0>(ptr);
        dst[1] = gather64_32<2>(ptr);
    }

    template <class T>
    __forceinline void gather64_64(const T* RESTRICT ptr, GSVector4i* RESTRICT dst) const
    {
        dst[0] = gather64_64<>(ptr);
    }

    __forceinline static GSVector4i loadnt(const void* p)
    {
        #if __has_builtin(__builtin_nontemporal_store)
        return GSVector4i(__builtin_nontemporal_load((v4i32*)p));
        #else
        return GSVector4i(__lsx_vld(p, 0));
        #endif
    }

    __forceinline static GSVector4i loadl(const void* p)
    {
        return GSVector4i(__lsx_vinsgr2vr_d(__lsx_vldi(0), *(const s64*)p, 0));
    }

    __forceinline static GSVector4i loadh(const void* p)
    {
        return GSVector4i(__lsx_vinsgr2vr_d(__lsx_vldi(0), *(const s64*)p, 1));
    }

    __forceinline static GSVector4i loadh(const void* p, const GSVector4i& v)
    {
        return GSVector4i(__lsx_vinsgr2vr_d(v.v4s, *(const s64*)p, 1));
    }

    __forceinline static GSVector4i loadh(const GSVector2i& v)
    {
        return loadh(&v);
    }

    __forceinline static GSVector4i load(const void* pl, const void* ph)
    {
        return GSVector4i(__lsx_vinsgr2vr_d(__lsx_vinsgr2vr_d(__lsx_vldi(0), *(const s64*)pl, 0), *(const s64*)ph, 1));
    }

    template <bool aligned>
    __forceinline static GSVector4i load(const void* p)
    {
        return GSVector4i(__lsx_vld(p, 0));
    }

    __forceinline static GSVector4i load(int i)
    {
        return GSVector4i(__lsx_vinsgr2vr_w(__lsx_vldi(0), i, 0));
    }

    __forceinline static GSVector4i loadq(s64 i)
    {
        return GSVector4i(__lsx_vinsgr2vr_d(__lsx_vldi(0), i, 0));
    }

    __forceinline static void storent(void* p, const GSVector4i& v)
    {
        #if __has_builtin(__builtin_nontemporal_store)
        __builtin_nontemporal_store(v.v4s, ((v4i32*)p));
        #else
        __lsx_vst(v.v4s, p, 0);
        #endif
    }

    __forceinline static void storel(void* p, const GSVector4i& v)
    {
        *(s64*)p = __lsx_vpickve2gr_d(v.v4s, 0);
    }

    __forceinline static void storeh(void* p, const GSVector4i& v)
    {
        *(s64*)p = __lsx_vpickve2gr_d(v.v4s, 1);
    }

    __forceinline static void store(void* pl, void* ph, const GSVector4i& v)
    {
        GSVector4i::storel(pl, v);
        GSVector4i::storeh(ph, v);
    }

    template <bool aligned>
    __forceinline static void store(void* p, const GSVector4i& v)
    {
        __lsx_vst(v.v4s, p, 0);
    }

    __forceinline static int store(const GSVector4i& v)
    {
        return __lsx_vpickve2gr_w(v.v4s, 0);
    }

    __forceinline static s64 storeq(const GSVector4i& v)
    {
        return __lsx_vpickve2gr_d(v.v4s, 0);
    }

    __forceinline static void storent(void* RESTRICT dst, const void* RESTRICT src, size_t size)
    {
        const GSVector4i* s = (const GSVector4i*)src;
        GSVector4i* d = (GSVector4i*)dst;

        if (size == 0)
            return;

        size_t i = 0;
        size_t j = size >> 6;

        for (; i < j; i++, s += 4, d += 4)
        {
            storent(&d[0], s[0]);
            storent(&d[1], s[1]);
            storent(&d[2], s[2]);
            storent(&d[3], s[3]);
        }

        size &= 63;

        if (size == 0)
            return;

        memcpy(d, s, size);
    }

    __forceinline static void mix4(GSVector4i& a, GSVector4i& b)
    {
        GSVector4i mask(__lsx_vreplgr2vr_w(0x0f0f0f0f));

        GSVector4i c = (b << 4).blend(a, mask);
        GSVector4i d = b.blend(a >> 4, mask);
        a = c;
        b = d;
    }

    __forceinline static void sw4(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
    {
        mix4(a, b);
        mix4(c, d);
        sw8(a, b, c, d);
    }

    __forceinline static void sw8(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
    {
        GSVector4i e = a;
        GSVector4i f = c;

        a = e.upl8(b);
        c = e.uph8(b);
        b = f.upl8(d);
        d = f.uph8(d);
    }

    __forceinline static void sw16(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
    {
        GSVector4i e = a;
        GSVector4i f = c;

        a = e.upl16(b);
        c = e.uph16(b);
        b = f.upl16(d);
        d = f.uph16(d);
    }

    __forceinline static void sw16rl(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
    {
        GSVector4i e = a;
        GSVector4i f = c;

        a = b.upl16(e);
        c = e.uph16(b);
        b = d.upl16(f);
        d = f.uph16(d);
    }

    __forceinline static void sw16rh(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
    {
        GSVector4i e = a;
        GSVector4i f = c;

        a = e.upl16(b);
        c = b.uph16(e);
        b = f.upl16(d);
        d = d.uph16(f);
    }

    __forceinline static void sw32(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
    {
        GSVector4i e = a;
        GSVector4i f = c;

        a = e.upl32(b);
        c = e.uph32(b);
        b = f.upl32(d);
        d = f.uph32(d);
    }

    __forceinline static void sw32_inv(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d);

    __forceinline static void sw64(GSVector4i& a, GSVector4i& b, GSVector4i& c, GSVector4i& d)
    {
        GSVector4i e = a;
        GSVector4i f = c;

        a = e.upl64(b);
        c = e.uph64(b);
        b = f.upl64(d);
        d = f.uph64(d);
    }

    __forceinline static bool compare16(const void* dst, const void* src, size_t size)
    {
        pxAssert((size & 15) == 0);

        size >>= 4;

        GSVector4i* s = (GSVector4i*)src;
        GSVector4i* d = (GSVector4i*)dst;

        for (size_t i = 0; i < size; i++)
        {
            if (!d[i].eq(s[i]))
            {
                return false;
            }
        }

        return true;
    }

    __forceinline static bool compare64(const void* dst, const void* src, size_t size)
    {
        pxAssert((size & 63) == 0);

        size >>= 6;

        GSVector4i* s = (GSVector4i*)src;
        GSVector4i* d = (GSVector4i*)dst;

        for (size_t i = 0; i < size; ++i)
        {
            GSVector4i v0 = (d[i * 4 + 0] == s[i * 4 + 0]);
            GSVector4i v1 = (d[i * 4 + 1] == s[i * 4 + 1]);
            GSVector4i v2 = (d[i * 4 + 2] == s[i * 4 + 2]);
            GSVector4i v3 = (d[i * 4 + 3] == s[i * 4 + 3]);

            v0 = v0 & v1;
            v2 = v2 & v3;

            if (!(v0 & v2).alltrue())
            {
                return false;
            }
        }

        return true;
    }

    __forceinline static bool update(const void* dst, const void* src, size_t size)
    {
        pxAssert((size & 15) == 0);

        size >>= 4;

        GSVector4i* s = (GSVector4i*)src;
        GSVector4i* d = (GSVector4i*)dst;

        GSVector4i v = GSVector4i::xffffffff();

        for (size_t i = 0; i < size; i++)
        {
            v &= d[i] == s[i];

            d[i] = s[i];
        }

        return v.alltrue();
    }

    __forceinline void operator+=(const GSVector4i& v)
    {
        v4s = __lsx_vadd_w(v4s, v.v4s);
    }

    __forceinline void operator-=(const GSVector4i& v)
    {
        v4s = __lsx_vsub_w(v4s, v.v4s);
    }

    __forceinline void operator+=(int i)
    {
        *this += GSVector4i(i);
    }

    __forceinline void operator-=(int i)
    {
        *this -= GSVector4i(i);
    }

    __forceinline void operator<<=(const int i)
    {
        v4s = __lsx_vsll_w(v4s, __lsx_vreplgr2vr_w(i));
    }

    __forceinline void operator>>=(const int i)
    {
        v4s = __lsx_vsrl_w(v4s, __lsx_vreplgr2vr_w(i));
    }

    __forceinline void operator&=(const GSVector4i& v)
    {
        v4s = __lsx_vand_v(v4s, v.v4s);
    }

    __forceinline void operator|=(const GSVector4i& v)
    {
        v4s = __lsx_vor_v(v4s, v.v4s);
    }

    __forceinline void operator^=(const GSVector4i& v)
    {
        v4s = __lsx_vxor_v(v4s, v.v4s);
    }

    __forceinline friend GSVector4i operator+(const GSVector4i& v1, const GSVector4i& v2)
    {
        return GSVector4i(__lsx_vadd_w(v1.v4s, v2.v4s));
    }

    __forceinline friend GSVector4i operator-(const GSVector4i& v1, const GSVector4i& v2)
    {
        return GSVector4i(__lsx_vsub_w(v1.v4s, v2.v4s));
    }

    __forceinline friend GSVector4i operator+(const GSVector4i& v, int i)
    {
        return v + GSVector4i(i);
    }

    __forceinline friend GSVector4i operator-(const GSVector4i& v, int i)
    {
        return v - GSVector4i(i);
    }

    __forceinline friend GSVector4i operator<<(const GSVector4i& v, const int i)
    {
        return GSVector4i(__lsx_vsll_w(v.v4s, __lsx_vreplgr2vr_w(i)));
    }

    __forceinline friend GSVector4i operator>>(const GSVector4i& v, const int i)
    {
        return GSVector4i(__lsx_vsrl_w(v.v4s, __lsx_vreplgr2vr_w(i)));
    }

    __forceinline friend GSVector4i operator&(const GSVector4i& v1, const GSVector4i& v2)
    {
        return GSVector4i(__lsx_vand_v(v1.v4s, v2.v4s));
    }

    __forceinline friend GSVector4i operator|(const GSVector4i& v1, const GSVector4i& v2)
    {
        return GSVector4i(__lsx_vor_v(v1.v4s, v2.v4s));
    }

    __forceinline friend GSVector4i operator^(const GSVector4i& v1, const GSVector4i& v2)
    {
        return GSVector4i(__lsx_vxor_v(v1.v4s, v2.v4s));
    }

    __forceinline friend GSVector4i operator&(const GSVector4i& v, int i)
    {
        return v & GSVector4i(i);
    }

    __forceinline friend GSVector4i operator|(const GSVector4i& v, int i)
    {
        return v | GSVector4i(i);
    }

    __forceinline friend GSVector4i operator^(const GSVector4i& v, int i)
    {
        return v ^ GSVector4i(i);
    }

    __forceinline friend GSVector4i operator-(const GSVector4i& v)
    {
        return GSVector4i(__lsx_vsub_w(__lsx_vldi(0), v.v4s));
    }

    __forceinline friend GSVector4i operator~(const GSVector4i& v)
    {
        return GSVector4i(__lsx_vnor_v(v.v4s, v.v4s));
    }

    __forceinline friend GSVector4i operator==(const GSVector4i& v1, const GSVector4i& v2)
    {
        return GSVector4i(__lsx_vseq_w(v1.v4s, v2.v4s));
    }

    __forceinline friend GSVector4i operator!=(const GSVector4i& v1, const GSVector4i& v2)
    {
        return ~(v1 == v2);
    }

    __forceinline friend GSVector4i operator>(const GSVector4i& v1, const GSVector4i& v2)
    {
        return GSVector4i(__lsx_vslt_w(v2.v4s, v1.v4s));
    }

    __forceinline friend GSVector4i operator<(const GSVector4i& v1, const GSVector4i& v2)
    {
        return GSVector4i(__lsx_vslt_w(v1.v4s, v2.v4s));
    }

    __forceinline friend GSVector4i operator>=(const GSVector4i& v1, const GSVector4i& v2)
    {
        return (v1 > v2) | (v1 == v2);
    }

    __forceinline friend GSVector4i operator<=(const GSVector4i& v1, const GSVector4i& v2)
    {
        return (v1 < v2) | (v1 == v2);
    }

    // clang-format off


    #define VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, ws, wn) \
    __forceinline GSVector4i xs##ys##zs##ws() const { return GSVector4i(__builtin_shufflevector(v4s, v4s, xn, yn, zn, wn)); }

    // __forceinline GSVector4i xs##ys##zs##ws() const {return GSVector4i(_mm_shuffle_epi32(m, _MM_SHUFFLE(wn, zn, yn, xn)));}
    // __forceinline GSVector4i xs##ys##zs##ws##l() const {return GSVector4i(_mm_shufflelo_epi16(m, _MM_SHUFFLE(wn, zn, yn, xn)));}
    // __forceinline GSVector4i xs##ys##zs##ws##h() const {return GSVector4i(_mm_shufflehi_epi16(m, _MM_SHUFFLE(wn, zn, yn, xn)));}
    // __forceinline GSVector4i xs##ys##zs##ws##lh() const {return GSVector4i(_mm_shufflehi_epi16(_mm_shufflelo_epi16(m, _MM_SHUFFLE(wn, zn, yn, xn)), _MM_SHUFFLE(wn, zn, yn, xn)));}

    #define VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, zs, zn) \
    VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, x, 0) \
    VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, y, 1) \
    VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, z, 2) \
    VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, w, 3) \

    #define VECTOR4i_SHUFFLE_2(xs, xn, ys, yn) \
    VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, x, 0) \
    VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, y, 1) \
    VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, z, 2) \
    VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, w, 3) \

    #define VECTOR4i_SHUFFLE_1(xs, xn) \
    VECTOR4i_SHUFFLE_2(xs, xn, x, 0) \
    VECTOR4i_SHUFFLE_2(xs, xn, y, 1) \
    VECTOR4i_SHUFFLE_2(xs, xn, z, 2) \
    VECTOR4i_SHUFFLE_2(xs, xn, w, 3) \

    VECTOR4i_SHUFFLE_1(x, 0)
    VECTOR4i_SHUFFLE_1(y, 1)
    VECTOR4i_SHUFFLE_1(z, 2)
    VECTOR4i_SHUFFLE_1(w, 3)

    // TODO: Make generic like above.
    __forceinline GSVector4i xxzzlh() const { return GSVector4i((v4i32)__builtin_shufflevector((v8i16)v4s, (v8i16)v4s, 0, 0, 2, 2, 4, 4, 6, 6)); }
    __forceinline GSVector4i yywwlh() const { return GSVector4i((v4i32)__builtin_shufflevector((v8i16)v4s, (v8i16)v4s, 1, 1, 3, 3, 5, 5, 7, 7)); }
    __forceinline GSVector4i yxwzlh() const { return GSVector4i((v4i32)__builtin_shufflevector((v8i16)v4s, (v8i16)v4s, 1, 0, 3, 2, 5, 4, 7, 6)); }
    __forceinline GSVector4i xxxxlh() const { return GSVector4i((v4i32)__builtin_shufflevector((v8i16)v4s, (v8i16)v4s, 0, 0, 0, 0, 4, 4, 4, 4)); }

    __forceinline GSVector4i xxxxl() const { return GSVector4i((v4i32)__builtin_shufflevector((v8i16)v4s, (v8i16)v4s, 0, 0, 0, 0, 4, 5, 6, 7)); }
    __forceinline GSVector4i zwxyl() const { return GSVector4i((v4i32)__builtin_shufflevector((v8i16)v4s, (v8i16)v4s, 2, 3, 0, 1, 4, 5, 6, 7)); }
    __forceinline GSVector4i yxwzl() const { return GSVector4i((v4i32)__builtin_shufflevector((v8i16)v4s, (v8i16)v4s, 1, 0, 3, 2, 4, 5, 6, 7)); }
    __forceinline GSVector4i zwzwl() const { return GSVector4i((v4i32)__builtin_shufflevector((v8i16)v4s, (v8i16)v4s, 2, 3, 2, 3, 4, 5, 6, 7)); }

    __forceinline GSVector4i zzzzh() const { return GSVector4i((v4i32)__builtin_shufflevector((v8i16)v4s, (v8i16)v4s, 0, 1, 2, 3, 6, 6, 6, 6)); }

    // clang-format on

    /// Noop, here so broadcast128 can be used generically over all vectors
    __forceinline static GSVector4i broadcast128(const GSVector4i& v)
    {
        return v;
    }

    __forceinline static GSVector4i broadcast16(u16 value)
    {
        return GSVector4i(__lsx_vreplgr2vr_h(value));
    }

    __forceinline static GSVector4i zero() { return GSVector4i(0); }

    __forceinline static GSVector4i xffffffff() { return GSVector4i(0xFFFFFFFF); }

    __forceinline static GSVector4i x00000001() { return xffffffff().srl32<31>(); }
    __forceinline static GSVector4i x00000003() { return xffffffff().srl32<30>(); }
    __forceinline static GSVector4i x00000007() { return xffffffff().srl32<29>(); }
    __forceinline static GSVector4i x0000000f() { return xffffffff().srl32<28>(); }
    __forceinline static GSVector4i x0000001f() { return xffffffff().srl32<27>(); }
    __forceinline static GSVector4i x0000003f() { return xffffffff().srl32<26>(); }
    __forceinline static GSVector4i x0000007f() { return xffffffff().srl32<25>(); }
    __forceinline static GSVector4i x000000ff() { return xffffffff().srl32<24>(); }
    __forceinline static GSVector4i x000001ff() { return xffffffff().srl32<23>(); }
    __forceinline static GSVector4i x000003ff() { return xffffffff().srl32<22>(); }
    __forceinline static GSVector4i x000007ff() { return xffffffff().srl32<21>(); }
    __forceinline static GSVector4i x00000fff() { return xffffffff().srl32<20>(); }
    __forceinline static GSVector4i x00001fff() { return xffffffff().srl32<19>(); }
    __forceinline static GSVector4i x00003fff() { return xffffffff().srl32<18>(); }
    __forceinline static GSVector4i x00007fff() { return xffffffff().srl32<17>(); }
    __forceinline static GSVector4i x0000ffff() { return xffffffff().srl32<16>(); }
    __forceinline static GSVector4i x0001ffff() { return xffffffff().srl32<15>(); }
    __forceinline static GSVector4i x0003ffff() { return xffffffff().srl32<14>(); }
    __forceinline static GSVector4i x0007ffff() { return xffffffff().srl32<13>(); }
    __forceinline static GSVector4i x000fffff() { return xffffffff().srl32<12>(); }
    __forceinline static GSVector4i x001fffff() { return xffffffff().srl32<11>(); }
    __forceinline static GSVector4i x003fffff() { return xffffffff().srl32<10>(); }
    __forceinline static GSVector4i x007fffff() { return xffffffff().srl32<9>(); }
    __forceinline static GSVector4i x00ffffff() { return xffffffff().srl32<8>(); }
    __forceinline static GSVector4i x01ffffff() { return xffffffff().srl32<7>(); }
    __forceinline static GSVector4i x03ffffff() { return xffffffff().srl32<6>(); }
    __forceinline static GSVector4i x07ffffff() { return xffffffff().srl32<5>(); }
    __forceinline static GSVector4i x0fffffff() { return xffffffff().srl32<4>(); }
    __forceinline static GSVector4i x1fffffff() { return xffffffff().srl32<3>(); }
    __forceinline static GSVector4i x3fffffff() { return xffffffff().srl32<2>(); }
    __forceinline static GSVector4i x7fffffff() { return xffffffff().srl32<1>(); }

    __forceinline static GSVector4i x80000000() { return xffffffff().sll32<31>(); }
    __forceinline static GSVector4i xc0000000() { return xffffffff().sll32<30>(); }
    __forceinline static GSVector4i xe0000000() { return xffffffff().sll32<29>(); }
    __forceinline static GSVector4i xf0000000() { return xffffffff().sll32<28>(); }
    __forceinline static GSVector4i xf8000000() { return xffffffff().sll32<27>(); }
    __forceinline static GSVector4i xfc000000() { return xffffffff().sll32<26>(); }
    __forceinline static GSVector4i xfe000000() { return xffffffff().sll32<25>(); }
    __forceinline static GSVector4i xff000000() { return xffffffff().sll32<24>(); }
    __forceinline static GSVector4i xff800000() { return xffffffff().sll32<23>(); }
    __forceinline static GSVector4i xffc00000() { return xffffffff().sll32<22>(); }
    __forceinline static GSVector4i xffe00000() { return xffffffff().sll32<21>(); }
    __forceinline static GSVector4i xfff00000() { return xffffffff().sll32<20>(); }
    __forceinline static GSVector4i xfff80000() { return xffffffff().sll32<19>(); }
    __forceinline static GSVector4i xfffc0000() { return xffffffff().sll32<18>(); }
    __forceinline static GSVector4i xfffe0000() { return xffffffff().sll32<17>(); }
    __forceinline static GSVector4i xffff0000() { return xffffffff().sll32<16>(); }
    __forceinline static GSVector4i xffff8000() { return xffffffff().sll32<15>(); }
    __forceinline static GSVector4i xffffc000() { return xffffffff().sll32<14>(); }
    __forceinline static GSVector4i xffffe000() { return xffffffff().sll32<13>(); }
    __forceinline static GSVector4i xfffff000() { return xffffffff().sll32<12>(); }
    __forceinline static GSVector4i xfffff800() { return xffffffff().sll32<11>(); }
    __forceinline static GSVector4i xfffffc00() { return xffffffff().sll32<10>(); }
    __forceinline static GSVector4i xfffffe00() { return xffffffff().sll32<9>(); }
    __forceinline static GSVector4i xffffff00() { return xffffffff().sll32<8>(); }
    __forceinline static GSVector4i xffffff80() { return xffffffff().sll32<7>(); }
    __forceinline static GSVector4i xffffffc0() { return xffffffff().sll32<6>(); }
    __forceinline static GSVector4i xffffffe0() { return xffffffff().sll32<5>(); }
    __forceinline static GSVector4i xfffffff0() { return xffffffff().sll32<4>(); }
    __forceinline static GSVector4i xfffffff8() { return xffffffff().sll32<3>(); }
    __forceinline static GSVector4i xfffffffc() { return xffffffff().sll32<2>(); }
    __forceinline static GSVector4i xfffffffe() { return xffffffff().sll32<1>(); }

    __forceinline static GSVector4i x0001() { return xffffffff().srl16<15>(); }
    __forceinline static GSVector4i x0003() { return xffffffff().srl16<14>(); }
    __forceinline static GSVector4i x0007() { return xffffffff().srl16<13>(); }
    __forceinline static GSVector4i x000f() { return xffffffff().srl16<12>(); }
    __forceinline static GSVector4i x001f() { return xffffffff().srl16<11>(); }
    __forceinline static GSVector4i x003f() { return xffffffff().srl16<10>(); }
    __forceinline static GSVector4i x007f() { return xffffffff().srl16<9>(); }
    __forceinline static GSVector4i x00ff() { return xffffffff().srl16<8>(); }
    __forceinline static GSVector4i x01ff() { return xffffffff().srl16<7>(); }
    __forceinline static GSVector4i x03ff() { return xffffffff().srl16<6>(); }
    __forceinline static GSVector4i x07ff() { return xffffffff().srl16<5>(); }
    __forceinline static GSVector4i x0fff() { return xffffffff().srl16<4>(); }
    __forceinline static GSVector4i x1fff() { return xffffffff().srl16<3>(); }
    __forceinline static GSVector4i x3fff() { return xffffffff().srl16<2>(); }
    __forceinline static GSVector4i x7fff() { return xffffffff().srl16<1>(); }

    __forceinline static GSVector4i x8000() { return xffffffff().sll16<15>(); }
    __forceinline static GSVector4i xc000() { return xffffffff().sll16<14>(); }
    __forceinline static GSVector4i xe000() { return xffffffff().sll16<13>(); }
    __forceinline static GSVector4i xf000() { return xffffffff().sll16<12>(); }
    __forceinline static GSVector4i xf800() { return xffffffff().sll16<11>(); }
    __forceinline static GSVector4i xfc00() { return xffffffff().sll16<10>(); }
    __forceinline static GSVector4i xfe00() { return xffffffff().sll16<9>(); }
    __forceinline static GSVector4i xff00() { return xffffffff().sll16<8>(); }
    __forceinline static GSVector4i xff80() { return xffffffff().sll16<7>(); }
    __forceinline static GSVector4i xffc0() { return xffffffff().sll16<6>(); }
    __forceinline static GSVector4i xffe0() { return xffffffff().sll16<5>(); }
    __forceinline static GSVector4i xfff0() { return xffffffff().sll16<4>(); }
    __forceinline static GSVector4i xfff8() { return xffffffff().sll16<3>(); }
    __forceinline static GSVector4i xfffc() { return xffffffff().sll16<2>(); }
    __forceinline static GSVector4i xfffe() { return xffffffff().sll16<1>(); }

    __forceinline static GSVector4i xffffffff(const GSVector4i& v) { return v == v; }

    __forceinline static GSVector4i x00000001(const GSVector4i& v) { return xffffffff(v).srl32<31>(); }
    __forceinline static GSVector4i x00000003(const GSVector4i& v) { return xffffffff(v).srl32<30>(); }
    __forceinline static GSVector4i x00000007(const GSVector4i& v) { return xffffffff(v).srl32<29>(); }
    __forceinline static GSVector4i x0000000f(const GSVector4i& v) { return xffffffff(v).srl32<28>(); }
    __forceinline static GSVector4i x0000001f(const GSVector4i& v) { return xffffffff(v).srl32<27>(); }
    __forceinline static GSVector4i x0000003f(const GSVector4i& v) { return xffffffff(v).srl32<26>(); }
    __forceinline static GSVector4i x0000007f(const GSVector4i& v) { return xffffffff(v).srl32<25>(); }
    __forceinline static GSVector4i x000000ff(const GSVector4i& v) { return xffffffff(v).srl32<24>(); }
    __forceinline static GSVector4i x000001ff(const GSVector4i& v) { return xffffffff(v).srl32<23>(); }
    __forceinline static GSVector4i x000003ff(const GSVector4i& v) { return xffffffff(v).srl32<22>(); }
    __forceinline static GSVector4i x000007ff(const GSVector4i& v) { return xffffffff(v).srl32<21>(); }
    __forceinline static GSVector4i x00000fff(const GSVector4i& v) { return xffffffff(v).srl32<20>(); }
    __forceinline static GSVector4i x00001fff(const GSVector4i& v) { return xffffffff(v).srl32<19>(); }
    __forceinline static GSVector4i x00003fff(const GSVector4i& v) { return xffffffff(v).srl32<18>(); }
    __forceinline static GSVector4i x00007fff(const GSVector4i& v) { return xffffffff(v).srl32<17>(); }
    __forceinline static GSVector4i x0000ffff(const GSVector4i& v) { return xffffffff(v).srl32<16>(); }
    __forceinline static GSVector4i x0001ffff(const GSVector4i& v) { return xffffffff(v).srl32<15>(); }
    __forceinline static GSVector4i x0003ffff(const GSVector4i& v) { return xffffffff(v).srl32<14>(); }
    __forceinline static GSVector4i x0007ffff(const GSVector4i& v) { return xffffffff(v).srl32<13>(); }
    __forceinline static GSVector4i x000fffff(const GSVector4i& v) { return xffffffff(v).srl32<12>(); }
    __forceinline static GSVector4i x001fffff(const GSVector4i& v) { return xffffffff(v).srl32<11>(); }
    __forceinline static GSVector4i x003fffff(const GSVector4i& v) { return xffffffff(v).srl32<10>(); }
    __forceinline static GSVector4i x007fffff(const GSVector4i& v) { return xffffffff(v).srl32<9>(); }
    __forceinline static GSVector4i x00ffffff(const GSVector4i& v) { return xffffffff(v).srl32<8>(); }
    __forceinline static GSVector4i x01ffffff(const GSVector4i& v) { return xffffffff(v).srl32<7>(); }
    __forceinline static GSVector4i x03ffffff(const GSVector4i& v) { return xffffffff(v).srl32<6>(); }
    __forceinline static GSVector4i x07ffffff(const GSVector4i& v) { return xffffffff(v).srl32<5>(); }
    __forceinline static GSVector4i x0fffffff(const GSVector4i& v) { return xffffffff(v).srl32<4>(); }
    __forceinline static GSVector4i x1fffffff(const GSVector4i& v) { return xffffffff(v).srl32<3>(); }
    __forceinline static GSVector4i x3fffffff(const GSVector4i& v) { return xffffffff(v).srl32<2>(); }
    __forceinline static GSVector4i x7fffffff(const GSVector4i& v) { return xffffffff(v).srl32<1>(); }

    __forceinline static GSVector4i x80000000(const GSVector4i& v) { return xffffffff(v).sll32<31>(); }
    __forceinline static GSVector4i xc0000000(const GSVector4i& v) { return xffffffff(v).sll32<30>(); }
    __forceinline static GSVector4i xe0000000(const GSVector4i& v) { return xffffffff(v).sll32<29>(); }
    __forceinline static GSVector4i xf0000000(const GSVector4i& v) { return xffffffff(v).sll32<28>(); }
    __forceinline static GSVector4i xf8000000(const GSVector4i& v) { return xffffffff(v).sll32<27>(); }
    __forceinline static GSVector4i xfc000000(const GSVector4i& v) { return xffffffff(v).sll32<26>(); }
    __forceinline static GSVector4i xfe000000(const GSVector4i& v) { return xffffffff(v).sll32<25>(); }
    __forceinline static GSVector4i xff000000(const GSVector4i& v) { return xffffffff(v).sll32<24>(); }
    __forceinline static GSVector4i xff800000(const GSVector4i& v) { return xffffffff(v).sll32<23>(); }
    __forceinline static GSVector4i xffc00000(const GSVector4i& v) { return xffffffff(v).sll32<22>(); }
    __forceinline static GSVector4i xffe00000(const GSVector4i& v) { return xffffffff(v).sll32<21>(); }
    __forceinline static GSVector4i xfff00000(const GSVector4i& v) { return xffffffff(v).sll32<20>(); }
    __forceinline static GSVector4i xfff80000(const GSVector4i& v) { return xffffffff(v).sll32<19>(); }
    __forceinline static GSVector4i xfffc0000(const GSVector4i& v) { return xffffffff(v).sll32<18>(); }
    __forceinline static GSVector4i xfffe0000(const GSVector4i& v) { return xffffffff(v).sll32<17>(); }
    __forceinline static GSVector4i xffff0000(const GSVector4i& v) { return xffffffff(v).sll32<16>(); }
    __forceinline static GSVector4i xffff8000(const GSVector4i& v) { return xffffffff(v).sll32<15>(); }
    __forceinline static GSVector4i xffffc000(const GSVector4i& v) { return xffffffff(v).sll32<14>(); }
    __forceinline static GSVector4i xffffe000(const GSVector4i& v) { return xffffffff(v).sll32<13>(); }
    __forceinline static GSVector4i xfffff000(const GSVector4i& v) { return xffffffff(v).sll32<12>(); }
    __forceinline static GSVector4i xfffff800(const GSVector4i& v) { return xffffffff(v).sll32<11>(); }
    __forceinline static GSVector4i xfffffc00(const GSVector4i& v) { return xffffffff(v).sll32<10>(); }
    __forceinline static GSVector4i xfffffe00(const GSVector4i& v) { return xffffffff(v).sll32<9>(); }
    __forceinline static GSVector4i xffffff00(const GSVector4i& v) { return xffffffff(v).sll32<8>(); }
    __forceinline static GSVector4i xffffff80(const GSVector4i& v) { return xffffffff(v).sll32<7>(); }
    __forceinline static GSVector4i xffffffc0(const GSVector4i& v) { return xffffffff(v).sll32<6>(); }
    __forceinline static GSVector4i xffffffe0(const GSVector4i& v) { return xffffffff(v).sll32<5>(); }
    __forceinline static GSVector4i xfffffff0(const GSVector4i& v) { return xffffffff(v).sll32<4>(); }
    __forceinline static GSVector4i xfffffff8(const GSVector4i& v) { return xffffffff(v).sll32<3>(); }
    __forceinline static GSVector4i xfffffffc(const GSVector4i& v) { return xffffffff(v).sll32<2>(); }
    __forceinline static GSVector4i xfffffffe(const GSVector4i& v) { return xffffffff(v).sll32<1>(); }

    __forceinline static GSVector4i x0001(const GSVector4i& v) { return xffffffff(v).srl16<15>(); }
    __forceinline static GSVector4i x0003(const GSVector4i& v) { return xffffffff(v).srl16<14>(); }
    __forceinline static GSVector4i x0007(const GSVector4i& v) { return xffffffff(v).srl16<13>(); }
    __forceinline static GSVector4i x000f(const GSVector4i& v) { return xffffffff(v).srl16<12>(); }
    __forceinline static GSVector4i x001f(const GSVector4i& v) { return xffffffff(v).srl16<11>(); }
    __forceinline static GSVector4i x003f(const GSVector4i& v) { return xffffffff(v).srl16<10>(); }
    __forceinline static GSVector4i x007f(const GSVector4i& v) { return xffffffff(v).srl16<9>(); }
    __forceinline static GSVector4i x00ff(const GSVector4i& v) { return xffffffff(v).srl16<8>(); }
    __forceinline static GSVector4i x01ff(const GSVector4i& v) { return xffffffff(v).srl16<7>(); }
    __forceinline static GSVector4i x03ff(const GSVector4i& v) { return xffffffff(v).srl16<6>(); }
    __forceinline static GSVector4i x07ff(const GSVector4i& v) { return xffffffff(v).srl16<5>(); }
    __forceinline static GSVector4i x0fff(const GSVector4i& v) { return xffffffff(v).srl16<4>(); }
    __forceinline static GSVector4i x1fff(const GSVector4i& v) { return xffffffff(v).srl16<3>(); }
    __forceinline static GSVector4i x3fff(const GSVector4i& v) { return xffffffff(v).srl16<2>(); }
    __forceinline static GSVector4i x7fff(const GSVector4i& v) { return xffffffff(v).srl16<1>(); }

    __forceinline static GSVector4i x8000(const GSVector4i& v) { return xffffffff(v).sll16<15>(); }
    __forceinline static GSVector4i xc000(const GSVector4i& v) { return xffffffff(v).sll16<14>(); }
    __forceinline static GSVector4i xe000(const GSVector4i& v) { return xffffffff(v).sll16<13>(); }
    __forceinline static GSVector4i xf000(const GSVector4i& v) { return xffffffff(v).sll16<12>(); }
    __forceinline static GSVector4i xf800(const GSVector4i& v) { return xffffffff(v).sll16<11>(); }
    __forceinline static GSVector4i xfc00(const GSVector4i& v) { return xffffffff(v).sll16<10>(); }
    __forceinline static GSVector4i xfe00(const GSVector4i& v) { return xffffffff(v).sll16<9>(); }
    __forceinline static GSVector4i xff00(const GSVector4i& v) { return xffffffff(v).sll16<8>(); }
    __forceinline static GSVector4i xff80(const GSVector4i& v) { return xffffffff(v).sll16<7>(); }
    __forceinline static GSVector4i xffc0(const GSVector4i& v) { return xffffffff(v).sll16<6>(); }
    __forceinline static GSVector4i xffe0(const GSVector4i& v) { return xffffffff(v).sll16<5>(); }
    __forceinline static GSVector4i xfff0(const GSVector4i& v) { return xffffffff(v).sll16<4>(); }
    __forceinline static GSVector4i xfff8(const GSVector4i& v) { return xffffffff(v).sll16<3>(); }
    __forceinline static GSVector4i xfffc(const GSVector4i& v) { return xffffffff(v).sll16<2>(); }
    __forceinline static GSVector4i xfffe(const GSVector4i& v) { return xffffffff(v).sll16<1>(); }

    __forceinline static GSVector4i xff(int n) { return m_xff[n]; }
    __forceinline static GSVector4i x0f(int n) { return m_x0f[n]; }
};
