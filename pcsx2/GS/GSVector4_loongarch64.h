// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0

class alignas(16) GSVector4
{
    struct cxpr_init_tag
    {
    };
    static constexpr cxpr_init_tag cxpr_init{};

    constexpr GSVector4(cxpr_init_tag, float x, float y, float z, float w)
    : F32{x, y, z, w}
    {
    }

    constexpr GSVector4(cxpr_init_tag, int x, int y, int z, int w)
    : I32{x, y, z, w}
    {
    }

    constexpr GSVector4(cxpr_init_tag, u64 x, u64 y)
    : U64{x, y}
    {
    }

public:
    union
    {
        struct { float x, y, z, w; };
        struct { float r, g, b, a; };
        struct { float left, top, right, bottom; };
        float v[4];
        float F32[4];
        double F64[2];
        s8 I8[16];
        s16 I16[8];
        s32 I32[4];
        s64 I64[2];
        u8 U8[16];
        u16 U16[8];
        u32 U32[4];
        u64 U64[2];
        v4f32 v4s;
    };

    static const GSVector4 m_ps0123;
    static const GSVector4 m_ps4567;
    static const GSVector4 m_half;
    static const GSVector4 m_one;
    static const GSVector4 m_two;
    static const GSVector4 m_four;
    static const GSVector4 m_x4b000000;
    static const GSVector4 m_x4f800000;
    static const GSVector4 m_xc1e00000000fffff;
    static const GSVector4 m_max;
    static const GSVector4 m_min;

    GSVector4() = default;

    constexpr static GSVector4 cxpr(float x, float y, float z, float w)
    {
        return GSVector4(cxpr_init, x, y, z, w);
    }

    constexpr static GSVector4 cxpr(float x)
    {
        return GSVector4(cxpr_init, x, x, x, x);
    }

    constexpr static GSVector4 cxpr(int x, int y, int z, int w)
    {
        return GSVector4(cxpr_init, x, y, z, w);
    }

    constexpr static GSVector4 cxpr(int x)
    {
        return GSVector4(cxpr_init, x, x, x, x);
    }

    constexpr static GSVector4 cxpr64(u64 x, u64 y)
    {
        return GSVector4(cxpr_init, x, y);
    }

    constexpr static GSVector4 cxpr64(u64 x)
    {
        return GSVector4(cxpr_init, x, x);
    }

    __forceinline GSVector4(float x, float y, float z, float w)
    {
        const float arr[4] = { x, y, z, w };
        v4s = __lsx_vld(arr, 0);
    }

    __forceinline GSVector4(float x, float y)
    {
        v4s = __lsx_vldi(0);
        v4s = __lsx_vinsgr2vr_w(v4s, std::bit_cast<int>(x), 0);
        v4s = __lsx_vinsgr2vr_w(v4s, std::bit_cast<int>(y), 1);
    }

    __forceinline GSVector4(int x, int y, int z, int w)
    {
        const int arr[4] = { x, y, z, w };
        v4s = __lsx_vffint_s_w(__lsx_vld(arr,0));
    }

    __forceinline GSVector4(int x, int y)
    {
        v4s = __lsx_vldi(0);
        v4s = __lsx_vinsgr2vr_w(v4s, x, 0);
        v4s = __lsx_vinsgr2vr_w(v4s, y, 1);
        v4s = __lsx_vffint_s_w(v4s);
    }

    __forceinline explicit GSVector4(const GSVector2& v)
    {
        v4s = (v4f32)__lsx_vinsgr2vr_w(__lsx_vldi(0), std::bit_cast<int>(v.x), 0);
        v4s = (v4f32)__lsx_vinsgr2vr_w((v4i32)v4s, std::bit_cast<int>(v.y), 1);
    }

    __forceinline explicit GSVector4(const GSVector2i& v)
    {
        v4s = (v4f32)__lsx_vinsgr2vr_w(__lsx_vldi(0), v.x, 0);
        v4s = (v4f32)__lsx_vinsgr2vr_w((v4i32)v4s, v.y, 1);
        v4s = __lsx_vffint_s_w((v4i32)v4s);
    }

    __forceinline constexpr explicit GSVector4(v4f32 m)
    : v4s(m)
    {
    }

    __forceinline explicit GSVector4(float f)
    {
        v4s = (v4f32)__lsx_vreplgr2vr_w(std::bit_cast<int>(f));
    }

    __forceinline explicit GSVector4(int i)
    {
        v4s = __lsx_vffint_s_w(__lsx_vreplgr2vr_w(i));
    }

    __forceinline explicit GSVector4(u32 u)
    {
        GSVector4i v((int)u);

        *this = GSVector4(v) + (m_x4f800000 & GSVector4::cast(v.sra32<31>()));
    }

    __forceinline explicit GSVector4(const GSVector4i& v);

    __forceinline static GSVector4 cast(const GSVector4i& v);

    __forceinline static GSVector4 f64(double x, double y)
    {
        return GSVector4((v4f32)__lsx_vinsgr2vr_d(__lsx_vinsgr2vr_d(__lsx_vldi(0), std::bit_cast<s64>(x), 0), std::bit_cast<s64>(y), 1));
    }

    __forceinline void operator=(float f)
    {
        v4s = (v4f32)__lsx_vreplgr2vr_w(std::bit_cast<int>(f));
    }

    __forceinline void operator=(v4f32 m)
    {
        v4s = m;
    }

    __forceinline operator v4f32() const
    {
        return v4s;
    }

    /// Makes Clang think that the whole vector is needed, preventing it from changing shuffles around because it thinks we don't need the whole vector
    /// Useful for e.g. preventing clang from optimizing shuffles that remove possibly-denormal garbage data from vectors before computing with them
    __forceinline GSVector4 noopt()
    {
        // Note: Clang is currently the only compiler that attempts to optimize vector intrinsics, if that changes in the future the implementation should be updated
        #ifdef __clang__
        // __asm__("":"+x"(m)::);
        #endif
        return *this;
    }

    __forceinline u32 rgba32() const
    {
        return GSVector4i(*this).rgba32();
    }

    __forceinline static GSVector4 rgba32(u32 rgba)
    {
        return GSVector4(GSVector4i::load((int)rgba).u8to32());
    }

    __forceinline static GSVector4 rgba32(u32 rgba, int shift)
    {
        return GSVector4(GSVector4i::load((int)rgba).u8to32() << shift);
    }

    __forceinline static GSVector4 unorm8(u32 rgba)
    {
        return rgba32(rgba) * GSVector4::cxpr(1.0f / 255.0f);
    }

    __forceinline GSVector4 abs() const
    {
        return GSVector4((v4f32)__lsx_vand_v((v4i32)v4s,__lsx_vreplgr2vr_w(0x7fffffff)));
    }

    __forceinline GSVector4 neg() const
    {
        return GSVector4((v4f32)__lsx_vxor_v((v4i32)v4s, __lsx_vreplgr2vr_w((int)0x80000000u)));
    }

    __forceinline GSVector4 rcp() const
    {
        return GSVector4(__lsx_vfrecip_s(v4s));
    }

    __forceinline GSVector4 rcpnr() const
    {
        v4f32 recip = __lsx_vfrecip_s(v4s);
        recip = __lsx_vfmul_s(recip, __lsx_vfsub_s(m_two.v4s, __lsx_vfmul_s(recip, v4s)));
        return GSVector4(recip);
    }

    template <int mode>
    __forceinline GSVector4 round() const
    {
        if constexpr (mode == Round_NegInf)
            return floor();
        else if constexpr (mode == Round_PosInf)
            return ceil();
        else if constexpr (mode == Round_NearestInt)
            return GSVector4(__lsx_vfrintrne_s(v4s));
        else
            return GSVector4(__lsx_vfrintrz_s(v4s));
    }

    __forceinline GSVector4 floor() const
    {
        return GSVector4(__lsx_vfrintrm_s(v4s));
    }

    __forceinline GSVector4 ceil() const
    {
        return GSVector4(__lsx_vfrintrp_s(v4s));
    }

    // http://jrfonseca.blogspot.com/2008/09/fast-sse2-pow-tables-or-polynomials.html

    #define LOG_POLY0(x, c0) GSVector4(c0)
    #define LOG_POLY1(x, c0, c1) (LOG_POLY0(x, c1).madd(x, GSVector4(c0)))
    #define LOG_POLY2(x, c0, c1, c2) (LOG_POLY1(x, c1, c2).madd(x, GSVector4(c0)))
    #define LOG_POLY3(x, c0, c1, c2, c3) (LOG_POLY2(x, c1, c2, c3).madd(x, GSVector4(c0)))
    #define LOG_POLY4(x, c0, c1, c2, c3, c4) (LOG_POLY3(x, c1, c2, c3, c4).madd(x, GSVector4(c0)))
    #define LOG_POLY5(x, c0, c1, c2, c3, c4, c5) (LOG_POLY4(x, c1, c2, c3, c4, c5).madd(x, GSVector4(c0)))

    __forceinline GSVector4 log2(int precision = 5) const
    {
        // NOTE: sign bit ignored, safe to pass negative numbers

        // The idea behind this algorithm is to split the float into two parts, log2(m * 2^e) => log2(m) + log2(2^e) => log2(m) + e,
        // and then approximate the logarithm of the mantissa (it's 1.x when normalized, a nice short range).

        GSVector4 one = m_one;

        GSVector4i i = GSVector4i::cast(*this);

        GSVector4 e = GSVector4(((i << 1) >> 24) - GSVector4i::x0000007f());
        GSVector4 m = GSVector4::cast((i << 9) >> 9) | one;

        GSVector4 p;

        // Minimax polynomial fit of log2(x)/(x - 1), for x in range [1, 2[

        switch (precision)
        {
            case 3:
                p = LOG_POLY2(m, 2.28330284476918490682f, -1.04913055217340124191f, 0.204446009836232697516f);
                break;
            case 4:
                p = LOG_POLY3(m, 2.61761038894603480148f, -1.75647175389045657003f, 0.688243882994381274313f, -0.107254423828329604454f);
                break;
            default:
            case 5:
                p = LOG_POLY4(m, 2.8882704548164776201f, -2.52074962577807006663f, 1.48116647521213171641f, -0.465725644288844778798f, 0.0596515482674574969533f);
                break;
            case 6:
                p = LOG_POLY5(m, 3.1157899f, -3.3241990f, 2.5988452f, -1.2315303f, 3.1821337e-1f, -3.4436006e-2f);
                break;
        }

        // This effectively increases the polynomial degree by one, but ensures that log2(1) == 0

        p = p * (m - one);

        return p + e;
    }

    __forceinline GSVector4 madd(const GSVector4& a, const GSVector4& b) const
    {
        return *this * a + b;
    }

    __forceinline GSVector4 msub(const GSVector4& a, const GSVector4& b) const
    {
        return *this * a - b;
    }

    __forceinline GSVector4 nmadd(const GSVector4& a, const GSVector4& b) const
    {
        return b - *this * a;
    }

    __forceinline GSVector4 nmsub(const GSVector4& a, const GSVector4& b) const
    {
        return -b - *this * a;
    }

    __forceinline GSVector4 addm(const GSVector4& a, const GSVector4& b) const
    {
        return a.madd(b, *this); // *this + a * b
    }

    __forceinline GSVector4 subm(const GSVector4& a, const GSVector4& b) const
    {
        return a.nmadd(b, *this); // *this - a * b
    }

    __forceinline GSVector4 hadd() const
    {
        return GSVector4(__builtin_shufflevector(v4s, v4s, 0, 2, 4, 6) + __builtin_shufflevector(v4s, v4s, 1, 3, 5, 7));
    }

    __forceinline GSVector4 hadd(const GSVector4& v) const
    {
        return GSVector4(__builtin_shufflevector(v4s, v.v4s, 0, 2, 4, 6) + __builtin_shufflevector(v4s, v.v4s, 1, 3, 5, 7));
    }

    __forceinline GSVector4 hsub() const
    {
        return GSVector4(__builtin_shufflevector(v4s, v4s, 0, 2, 4, 6) - __builtin_shufflevector(v4s, v4s, 1, 3, 5, 7));
    }

    __forceinline GSVector4 hsub(const GSVector4& v) const
    {
        return GSVector4(__builtin_shufflevector(v4s, v.v4s, 0, 2, 4, 6) - __builtin_shufflevector(v4s, v.v4s, 1, 3, 5, 7));
    }

    __forceinline GSVector4 sat(const GSVector4& a, const GSVector4& b) const
    {
        return max(a).min(b);
    }

    __forceinline GSVector4 sat(const GSVector4& a) const
    {
        const GSVector4 minv(__builtin_shufflevector(a.v4s, a.v4s, 0, 1, 0, 1));
        const GSVector4 maxv(__builtin_shufflevector(a.v4s, a.v4s, 2, 3, 2, 3));
        return sat(minv, maxv);
    }

    __forceinline GSVector4 sat(const float scale = 255) const
    {
        return sat(zero(), GSVector4(scale));
    }

    __forceinline GSVector4 clamp(const float scale = 255) const
    {
        return min(GSVector4(scale));
    }

    __forceinline GSVector4 min(const GSVector4& a) const
    {
        return GSVector4(__lsx_vfmin_s(v4s, a.v4s));
    }

    __forceinline GSVector4 max(const GSVector4& a) const
    {
        return GSVector4(__lsx_vfmax_s(v4s, a.v4s));
    }

    template <int mask>
    __forceinline GSVector4 blend32(const GSVector4& a) const
    {
        return GSVector4(__builtin_shufflevector(v4s, a.v4s, (mask & 1) ? 4 : 0, (mask & 2) ? 5 : 1, (mask & 4) ? 6 : 2, (mask & 8) ? 7 : 3));
    }

    __forceinline GSVector4 blend32(const GSVector4& a, const GSVector4& mask) const
    {
        v4i32 bitmask = __lsx_vsrai_w((v4i32)mask.v4s, 31);
        return GSVector4((v4f32)__lsx_vbitsel_v((v4i32)v4s, (v4i32)a.v4s, bitmask));
    }

    __forceinline GSVector4 upl(const GSVector4& a) const
    {
        return GSVector4((v4f32)__lsx_vilvl_w((v4i32)a.v4s, (v4i32)v4s));
    }

    __forceinline GSVector4 uph(const GSVector4& a) const
    {
        return GSVector4((v4f32)__lsx_vilvh_w((v4i32)a.v4s, (v4i32)v4s));
    }

    __forceinline GSVector4 upld(const GSVector4& a) const
    {
        return GSVector4((v4f32)__lsx_vilvl_d((v4i32)a.v4s, (v4i32)v4s));
    }

    __forceinline GSVector4 uphd(const GSVector4& a) const
    {
        return GSVector4((v4f32)__lsx_vilvh_d((v4i32)a.v4s, (v4i32)v4s));
    }

    __forceinline GSVector4 l2h(const GSVector4& a) const
    {
        return GSVector4((v4f32)__lsx_vilvl_d((v4i32)a.v4s, (v4i32)v4s));
    }

    __forceinline GSVector4 h2l(const GSVector4& a) const
    {
        return GSVector4((v4f32)__lsx_vilvh_d((v4i32)v4s, (v4i32)a.v4s));
    }

    __forceinline GSVector4 andnot(const GSVector4& v) const
    {
        return GSVector4((v4f32)__lsx_vandn_v((v4i32)v.v4s, (v4i32)v4s));
    }

    __forceinline int mask() const
    {
        return __lsx_vpickve2gr_w(__lsx_vmskltz_w((v4i32)v4s), 0);
    }

    __forceinline bool alltrue() const
    {
        return mask() == 0xf;
    }

    __forceinline bool allfalse() const
    {
        return (U64[0] | U64[1]) == 0;
    }

    __forceinline GSVector4 replace_nan(const GSVector4& v) const
    {
        return v.blend32(*this, *this == *this);
    }

    template <int src, int dst>
    __forceinline GSVector4 insert32(const GSVector4& v) const
    {
        return GSVector4((v4f32)__lsx_vinsgr2vr_w((v4i32)v4s, __lsx_vpickve2gr_w((v4i32)v.v4s, src), dst));
    }

    template <int i>
    __forceinline int extract32() const
    {
        return __lsx_vpickve2gr_w((v4i32)v4s, i);
    }

    __forceinline static GSVector4 zero()
    {
        return GSVector4((v4f32)__lsx_vldi(0));
    }

    __forceinline static GSVector4 xffffffff()
    {
        return GSVector4((v4f32)__lsx_vldi(-1));
    }

    __forceinline static GSVector4 ps0123()
    {
        return GSVector4(m_ps0123);
    }

    __forceinline static GSVector4 ps4567()
    {
        return GSVector4(m_ps4567);
    }

    __forceinline static GSVector4 loadl(const void* p)
    {
        return GSVector4((v4f32)__lsx_vinsgr2vr_d(__lsx_vldi(0), *(const s64*)p, 0));
    }

    __forceinline static GSVector4 load(float f)
    {
        return GSVector4((v4f32)__lsx_vinsgr2vr_w(__lsx_vldi(0), std::bit_cast<int>(f), 0));
    }

    __forceinline static GSVector4 load(u32 u)
    {
        GSVector4i v = GSVector4i::load((int)u);

        return GSVector4(v) + (m_x4f800000 & GSVector4::cast(v.sra32<31>()));
    }

    template <bool aligned>
    __forceinline static GSVector4 load(const void* p)
    {
        return GSVector4((v4f32)__lsx_vld(p, 0));
    }

    __forceinline static void storent(void* p, const GSVector4& v)
    {
        __lsx_vst((v4i32)v.v4s, p, 0);
    }

    __forceinline static void storel(void* p, const GSVector4& v)
    {
        *(s64*)p = __lsx_vpickve2gr_d((v4i32)v.v4s, 0);
    }

    __forceinline static void storeh(void* p, const GSVector4& v)
    {
        *(s64*)p = __lsx_vpickve2gr_d((v4i32)v.v4s, 1);
    }

    template <bool aligned>
    __forceinline static void store(void* p, const GSVector4& v)
    {
        __lsx_vst((v4i32)v.v4s, p, 0);
    }

    __forceinline static void store(float* p, const GSVector4& v)
    {
        *p = std::bit_cast<float>(__lsx_vpickve2gr_w((v4i32)v.v4s, 0));
    }

    __forceinline static void expand(const GSVector4i& v, GSVector4& a, GSVector4& b, GSVector4& c, GSVector4& d)
    {
        GSVector4i mask = GSVector4i::x000000ff();

        a = GSVector4(v & mask);
        b = GSVector4((v >> 8) & mask);
        c = GSVector4((v >> 16) & mask);
        d = GSVector4((v >> 24));
    }

    __forceinline static void transpose(GSVector4& a, GSVector4& b, GSVector4& c, GSVector4& d)
    {
        GSVector4 v0 = a.xyxy(b);
        GSVector4 v1 = c.xyxy(d);

        GSVector4 e = v0.xzxz(v1);
        GSVector4 f = v0.ywyw(v1);

        GSVector4 v2 = a.zwzw(b);
        GSVector4 v3 = c.zwzw(d);

        GSVector4 g = v2.xzxz(v3);
        GSVector4 h = v2.ywyw(v3);

        a = e;
        b = f;
        c = g;
        d = h;
    }

    __forceinline GSVector4 operator-() const
    {
        return neg();
    }

    __forceinline void operator+=(const GSVector4& v)
    {
        v4s = __lsx_vfadd_s(v4s, v.v4s);
    }

    __forceinline void operator-=(const GSVector4& v)
    {
        v4s = __lsx_vfsub_s(v4s, v.v4s);
    }

    __forceinline void operator*=(const GSVector4& v)
    {
        v4s = __lsx_vfmul_s(v4s, v.v4s);
    }

    __forceinline void operator/=(const GSVector4& v)
    {
        v4s = __lsx_vfdiv_s(v4s, v.v4s);
    }

    __forceinline void operator+=(float f)
    {
        *this += GSVector4(f);
    }

    __forceinline void operator-=(float f)
    {
        *this -= GSVector4(f);
    }

    __forceinline void operator*=(float f)
    {
        *this *= GSVector4(f);
    }

    __forceinline void operator/=(float f)
    {
        *this /= GSVector4(f);
    }

    __forceinline void operator&=(const GSVector4& v)
    {
        v4s = (v4f32)__lsx_vand_v((v4i32)v4s, (v4i32)v.v4s);
    }

    __forceinline void operator|=(const GSVector4& v)
    {
        v4s = (v4f32)__lsx_vor_v((v4i32)v4s, (v4i32)v.v4s);
    }

    __forceinline void operator^=(const GSVector4& v)
    {
        v4s = (v4f32)__lsx_vxor_v((v4i32)v4s, (v4i32)v.v4s);
    }

    __forceinline friend GSVector4 operator+(const GSVector4& v1, const GSVector4& v2)
    {
        return GSVector4(__lsx_vfadd_s(v1.v4s, v2.v4s));
    }

    __forceinline friend GSVector4 operator-(const GSVector4& v1, const GSVector4& v2)
    {
        return GSVector4(__lsx_vfsub_s(v1.v4s, v2.v4s));
    }

    __forceinline friend GSVector4 operator*(const GSVector4& v1, const GSVector4& v2)
    {
        return GSVector4(__lsx_vfmul_s(v1.v4s, v2.v4s));
    }

    __forceinline friend GSVector4 operator/(const GSVector4& v1, const GSVector4& v2)
    {
        return GSVector4(__lsx_vfdiv_s(v1.v4s, v2.v4s));
    }

    __forceinline friend GSVector4 operator+(const GSVector4& v, float f)
    {
        return v + GSVector4(f);
    }

    __forceinline friend GSVector4 operator-(const GSVector4& v, float f)
    {
        return v - GSVector4(f);
    }

    __forceinline friend GSVector4 operator*(const GSVector4& v, float f)
    {
        return v * GSVector4(f);
    }

    __forceinline friend GSVector4 operator/(const GSVector4& v, float f)
    {
        return v / GSVector4(f);
    }

    __forceinline friend GSVector4 operator&(const GSVector4& v1, const GSVector4& v2)
    {
        return GSVector4((v4f32)__lsx_vand_v((v4i32)v1.v4s, (v4i32)v2.v4s));
    }

    __forceinline friend GSVector4 operator|(const GSVector4& v1, const GSVector4& v2)
    {
        return GSVector4((v4f32)__lsx_vor_v((v4i32)v1.v4s, (v4i32)v2.v4s));
    }

    __forceinline friend GSVector4 operator^(const GSVector4& v1, const GSVector4& v2)
    {
        return GSVector4((v4f32)__lsx_vxor_v((v4i32)v1.v4s, (v4i32)v2.v4s));
    }

    __forceinline friend GSVector4 operator==(const GSVector4& v1, const GSVector4& v2)
    {
        return GSVector4((v4f32)__lsx_vfcmp_ceq_s(v1.v4s, v2.v4s));
    }

    __forceinline friend GSVector4 operator!=(const GSVector4& v1, const GSVector4& v2)
    {
        return GSVector4((v4f32)__lsx_vnor_v(__lsx_vfcmp_ceq_s(v1.v4s, v2.v4s), __lsx_vfcmp_ceq_s(v1.v4s, v2.v4s)));
    }

    __forceinline friend GSVector4 operator>(const GSVector4& v1, const GSVector4& v2)
    {
        return GSVector4((v4f32)__lsx_vfcmp_clt_s(v2.v4s, v1.v4s));
    }

    __forceinline friend GSVector4 operator<(const GSVector4& v1, const GSVector4& v2)
    {
        return GSVector4((v4f32)__lsx_vfcmp_clt_s(v1.v4s, v2.v4s));
    }

    __forceinline friend GSVector4 operator>=(const GSVector4& v1, const GSVector4& v2)
    {
        return GSVector4((v4f32)__lsx_vfcmp_cle_s(v2.v4s, v1.v4s));
    }

    __forceinline friend GSVector4 operator<=(const GSVector4& v1, const GSVector4& v2)
    {
        return GSVector4((v4f32)__lsx_vfcmp_cle_s(v1.v4s, v2.v4s));
    }

    __forceinline GSVector4 mul64(const GSVector4& v) const
    {
        return GSVector4((v4f32)__lsx_vfmul_d((v2f64)v4s, (v2f64)v.v4s));
    }

    __forceinline GSVector4 add64(const GSVector4& v) const
    {
        return GSVector4((v4f32)__lsx_vfadd_d((v2f64)v4s, (v2f64)v.v4s));
    }

    __forceinline GSVector4 sub64(const GSVector4& v) const
    {
        return GSVector4((v4f32)__lsx_vfsub_d((v2f64)v4s, (v2f64)v.v4s));
    }

    __forceinline static GSVector4 f32to64(const GSVector4& v)
    {
        return GSVector4((v4f32)__lsx_vfcvtl_d_s(v.v4s));
    }

    __forceinline static GSVector4 f32to64(const void* p)
    {
        return GSVector4((v4f32)__lsx_vfcvtl_d_s((v4f32)__lsx_vinsgr2vr_d(__lsx_vldi(0), *(const s64*)p, 0)));
    }

    __forceinline GSVector4i f64toi32(bool truncate = true) const
    {
        v4i32 r = truncate ? __lsx_vftintrz_l_d((v2f64)v4s) : __lsx_vftintrne_l_d((v2f64)v4s);
        return GSVector4i(__builtin_shufflevector(r, (v4i32)__lsx_vldi(0), 0, 2, 4, 5));
    }

    // clang-format off

    #define VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, ws, wn) \
    __forceinline GSVector4 xs##ys##zs##ws() const { return GSVector4(__builtin_shufflevector(v4s, v4s, xn, yn, zn, wn)); } \
    __forceinline GSVector4 xs##ys##zs##ws(const GSVector4& v) const { return GSVector4(__builtin_shufflevector(v4s, v.v4s, xn, yn, 4 + zn, 4 + wn)); }

    #define VECTOR4_SHUFFLE_3(xs, xn, ys, yn, zs, zn) \
    VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, x, 0) \
    VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, y, 1) \
    VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, z, 2) \
    VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, w, 3) \

    #define VECTOR4_SHUFFLE_2(xs, xn, ys, yn) \
    VECTOR4_SHUFFLE_3(xs, xn, ys, yn, x, 0) \
    VECTOR4_SHUFFLE_3(xs, xn, ys, yn, y, 1) \
    VECTOR4_SHUFFLE_3(xs, xn, ys, yn, z, 2) \
    VECTOR4_SHUFFLE_3(xs, xn, ys, yn, w, 3) \

    #define VECTOR4_SHUFFLE_1(xs, xn) \
    VECTOR4_SHUFFLE_2(xs, xn, x, 0) \
    VECTOR4_SHUFFLE_2(xs, xn, y, 1) \
    VECTOR4_SHUFFLE_2(xs, xn, z, 2) \
    VECTOR4_SHUFFLE_2(xs, xn, w, 3) \

    VECTOR4_SHUFFLE_1(x, 0)
    VECTOR4_SHUFFLE_1(y, 1)
    VECTOR4_SHUFFLE_1(z, 2)
    VECTOR4_SHUFFLE_1(w, 3)

    // clang-format on

    __forceinline GSVector4 broadcast32() const
    {
        return GSVector4((v4f32)__lsx_vreplvei_w((v4i32)v4s, 0));
    }

    __forceinline static GSVector4 broadcast32(const GSVector4& v)
    {
        return GSVector4((v4f32)__lsx_vreplvei_w((v4i32)v.v4s, 0));
    }

    __forceinline static GSVector4 broadcast32(const void* f)
    {
        return GSVector4((v4f32)__lsx_vreplgr2vr_w(*(const int*)f));
    }

    __forceinline static GSVector4 broadcast64(const void* f)
    {
        return GSVector4((v4f32)__lsx_vreplgr2vr_d(*(const s64*)f));
    }
};
