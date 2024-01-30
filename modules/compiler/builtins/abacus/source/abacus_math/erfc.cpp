// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <abacus/abacus_config.h>
#include <abacus/abacus_math.h>
#include <abacus/internal/add_exact.h>
#include <abacus/internal/exp_unsafe.h>
#include <abacus/internal/horner_polynomial.h>
#include <abacus/internal/multiply_exact.h>
#include <abacus/internal/multiply_exact_unsafe.h>

namespace {
template <typename T, typename E = typename TypeTraits<T>::ElementType>
struct erfc_helper;

#ifdef __CA_BUILTINS_HALF_SUPPORT
template <typename T>
struct erfc_helper<T, abacus_half> {
  using SignedType = typename TypeTraits<T>::SignedType;

  static T _(const T x) {
    const T xAbs = __abacus_fabs(x);

    // Polynomial approximations across various input thresholds.
    // See erfc.sollya for the derivations.

    // Polynomial of erfc() for range [0, 0.8]
    const abacus_half polynomial0[4] = {1.0009765625f16, -1.15625f16,
                                        0.1346435546875f16, 0.1875f16};
    const T s0 = abacus::internal::horner_polynomial(xAbs, polynomial0);

    // Polynomial approximation of 'erfc(x) * x^2 * exp(x^2)' over [0.8, 1.75]
    const abacus_half polynomial1[4] = {-9.47265625e-2f16, 0.439453125f16,
                                        0.1070556640625f16,
                                        -2.41851806640625e-2f16};
    const T s1 = abacus::internal::horner_polynomial(xAbs, polynomial1);

    // Polynomial approximation of 'erfc(x) * x^2 * exp(x^2)' over [1.75, 2.5]
    const abacus_half polynomial2[3] = {-0.1822509765625f16, 0.609375f16,
                                        -3.73077392578125e-3f16};
    const T s2 = abacus::internal::horner_polynomial(xAbs, polynomial2);

    // Select the last interval as the default value
    T result = s2;
    result = __abacus_select(result, s1, SignedType(xAbs < 1.75f16));
    result = __abacus_select(result, s0, SignedType(xAbs < 0.8f16));

    // For xAbs > 0.8 multiply polynomial by '1 / (x^2 * exp(x^2))'
    // which can be transformed into 'exp(-(x^2)) / x^2'
    T x_2_lo;
    const T x_2_hi = abacus::internal::multiply_exact(x, x, &x_2_lo);

    const T exp_hi = abacus::internal::exp_unsafe(-x_2_hi);
    const T exp_lo = abacus::internal::exp_unsafe(-x_2_lo);

    // Ordering is important here, doing `(exp_hi * exp_lo) / x_2_hi` before
    // multiplying by `result` can lead to an intermediate denormal number,
    // which not all devices support.
    T bit = exp_hi * exp_lo;
    bit *= result / x_2_hi;

    result = __abacus_select(result, bit, SignedType(xAbs >= 0.8f16));

    result = __abacus_select(result, T(2.0f16) - result,
                             SignedType(__abacus_signbit(x)));

    result = __abacus_select(result, T(2.0f16), SignedType(x <= -2.1f16));

    result = __abacus_select(result, T(0.0f16), SignedType(x >= 3.7f16));

    return result;
  }
};
#endif  // __CA_BUILTINS_HALF_SUPPORT

template <typename T>
struct erfc_helper<T, abacus_float> {
  static T _(const T x) {
    const T xAbs = __abacus_fabs(x);

    // Polynomial approximation of erfc(x) for range [0, 0.8]
    const abacus_float polynomial0[8] = {
        1.000000099f,    -1.128392934f, 0.3227568e-3f,    .3731939303f,
        0.131956151e-1f, -.1449755580f, 0.4206089062e-1f, 0.1936482390e-2f,
    };
    const T s0 = abacus::internal::horner_polynomial(xAbs, polynomial0);

    // Polynomial approximation of 'erfc(x) * x^2 * exp(x^2)' over [0.8, 2.0]
    const abacus_float polynomial1[8] = {-0.1195251196e-1f, 0.858939377e-1f,
                                         0.7237242471e0f,   -0.5984700845e0f,
                                         0.3146238633e0f,   -0.1046465402e0f,
                                         0.2011317702e-1f,  -0.1702538918e-2f};
    const T s1 = abacus::internal::horner_polynomial(xAbs, polynomial1);

    // Polynomial approximation of 'erfc(x) * x^2 * exp(x^2)' over [2.0, 4.5]
    const abacus_float polynomial2[8] = {-0.1314926638e0f, 0.4923351369e0f,
                                         0.1059452967e0f,  -0.5427770214e-1f,
                                         0.1550428873e-1f, -0.2610687156e-2f,
                                         0.2431079121e-3f, -0.9688281705e-5f};
    const T s2 = abacus::internal::horner_polynomial(xAbs, polynomial2);

    // Polynomial approximation of 'erfc(x) * x^2 * exp(x^2)' over [4.5, 10]
    const abacus_float polynomial3[8] = {-0.2132052659e0f,  0.6462258403e0f,
                                         -0.1951384942e-1f, 0.3032789666e-2f,
                                         -0.3095515185e-3f, 0.2004225582e-4f,
                                         -0.7474507440e-6f, 0.1223817901e-7f};

    const T s3 = abacus::internal::horner_polynomial(xAbs, polynomial3);

    // Select the last interval as the default value
    T result = s3;
    result = __abacus_select(result, s2, xAbs < 4.5f);
    result = __abacus_select(result, s1, xAbs < 2.0f);
    result = __abacus_select(result, s0, xAbs < 0.8f);

    // For xAbs > 0.8 multiply polynomial by '1 / (x^2 * exp(x^2))'
    // which can be transformed into 'exp(-(x^2)) / x^2'
    T x_2_lo;
    const T x_2_hi = abacus::internal::multiply_exact(x, x, &x_2_lo);

    const T bit = abacus::internal::exp_unsafe(-x_2_hi) *
                  abacus::internal::exp_unsafe(-x_2_lo) / x_2_hi;

    result = __abacus_select(result, result * bit, xAbs >= 0.8f);

    result = __abacus_select(result, T(2.0f) - result, __abacus_signbit(x));

    result = __abacus_select(result, 2.0f, x <= -3.8f);

    result = __abacus_select(result, 0.0f, x >= 10.0f);

    return result;
  }
};

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
template <typename T>
struct erfc_helper<T, abacus_double> {
  using SignedType = typename TypeTraits<T>::SignedType;

  static T _(const T x) {
    const T xAbs = __abacus_fabs(x);

    // see erfc.maple worksheet for polynomial derivations
    const abacus_double polynomial0[16] = {
        0.22549572432641358943602822490295805141535164042853e-1,
        0.90054546344462671592964886621593776413212630385706e-3,
        0.35935846525691045418320494700392904187241924007907e-4,
        0.14328668682337212559839791383793311090600423365962e-5,
        0.57087409924006711577319160466832208298271111075605e-7,
        0.22726480534184159537119324507180518699109413647034e-8,
        0.90402862849366485879463182875826284069384104282104e-10,
        0.35932806283231170989802647498162645124872673795474e-11,
        0.14271178473719403328435140495418125794766129600168e-12,
        0.56635545018844478258694915545436259767145368311395e-14,
        0.22458474999041707733741317129950959093318887525135e-15,
        0.88988308717384178027054837871505823114159433681031e-17,
        0.35223752467781089420757064116725398249437508851672e-18,
        0.13935278414864926746629392212055140999082019366302e-19,
        0.56497573738414691546879369619498766617327444826545e-21,
        0.22315411099578621908783495598284443407283662209588e-22};

    const T s0 =
        abacus::internal::horner_polynomial((T)25.0 - xAbs, polynomial0);

    const abacus_double polynomial1[16] = {
        0.26835813158647956642164066632119246357135547849751e-1,
        0.12750144322983949238889048072643776016973505401660e-2,
        0.60510080381663241556253645728168985793862631706936e-4,
        0.28684961889779155500290601719653081101741353032550e-5,
        0.13583020656350194190217163484804974327338320783976e-6,
        0.64247404577033557824518042492806321591467325291641e-8,
        0.30355231725494562249949878915271029671684303439812e-9,
        0.14326227309367675187428929915517929925251243495299e-10,
        0.67538592863235180019094477093756479526680875807571e-12,
        0.31805017432559767108532240149322306270027126618542e-13,
        0.14961183358295113077651649300343510615897948946566e-14,
        0.70300933143226721246519930266578815780518093412700e-16,
        0.32980685931579685362518999147778846763979960252051e-17,
        0.15463743087766322211203650857284048036199734325615e-18,
        0.75051942738631207938322427101220051545479451139095e-20,
        0.35108463749050907584106554056394450250981113580702e-21};

    const T s1 =
        abacus::internal::horner_polynomial((T)21.0 - xAbs, polynomial1);

    const abacus_double polynomial2[16] = {
        0.33130499999725536698661233824251148061300199979814e-1,
        0.19421671048443261038468720385862210701521635921412e-2,
        0.11365921737199296930280897591004113515988840803210e-3,
        0.66402730136308220595180948574838392579757215906556e-5,
        0.38728807013430792983999516325223656194540315190047e-6,
        0.22550328537754952140068994712235717418081205871496e-7,
        0.13108283312030350393351019477039524201695007518527e-8,
        0.76070546778642923158190129579953255842376671640767e-10,
        0.44072586198123770191912832983243691004805590351902e-11,
        0.25492086952420801558196730458541019865120321855319e-12,
        0.14720965835710104925666516077732095889852472368389e-13,
        0.84869770436148822902993422581227986862978488318673e-15,
        0.48792232731553936788245882285861298970401765895942e-16,
        0.28039381326683836348878962699132702159518824095216e-17,
        0.16976712067638702328856389496243970673000579929844e-18,
        0.97212298021469650250629966132938751449592339693735e-20};

    const T s2 =
        abacus::internal::horner_polynomial((T)17.0 - xAbs, polynomial2);

    const abacus_double polynomial3[16] = {
        0.43271921864609692570782914322398106742898737622031e-1,
        0.33091986156605646511264121203475958669051883129775e-2,
        0.25233986102235512767975117901916431178732211510789e-3,
        0.19186948246657797624374970221499496602645431403253e-4,
        0.14547669078860129208636455079517611951743724943653e-5,
        0.10999137757408713501687621415456238282028340428372e-6,
        0.82929998393647032269683448281011781218900713134162e-8,
        0.62353716388288060450397617310469951059979056968730e-9,
        0.46754146046341757710964628301867388652682592174889e-10,
        0.34961908893632142558435854601305935448732483273879e-11,
        0.26074952573199992720822804369252479373597628643450e-12,
        0.19393637281853324910672311883556189887346829365560e-13,
        0.14337933143096218027584792095063747638465540067122e-14,
        0.10608538370677248951510891903879140885654286715103e-15,
        0.85678940245325203804453120303120237859651760770720e-17,
        0.62999013306513108998342864875729309153681534750350e-18};

    const T s3 =
        abacus::internal::horner_polynomial((T)13.0 - xAbs, polynomial3);

    const abacus_double polynomial4[16] = {
        0.62307724037774650307656620378742578230490125142309e-1,
        0.68401344155682605053412231385312868831305938176525e-2,
        0.74651429766143431141588988277120884710280270522952e-3,
        0.81003824416800218845343512435084597434467365058894e-4,
        0.87399389491865494488529138045387604729491200567899e-5,
        0.93774952870763700266625411545410203828331644621019e-6,
        0.10006440880613165437557222037009546177182296416585e-6,
        0.10619987272113710039816168891384700569434163597580e-7,
        0.11211191781687605647083653714115177650907117714276e-8,
        0.11773539728159165934013413950062574833018967661795e-9,
        0.12306342534222895681221718540598840519951391937921e-10,
        0.12791268051360968102168551658441850107787874234615e-11,
        0.13052787087879926869402538688953937708157453528511e-12,
        0.13441331880464779669943337162525117250967781936410e-13,
        0.16481422678766834003772364764728548729362102996262e-14,
        0.16739939975355356338814822591192186560474315370372e-15};

    const T s4 =
        abacus::internal::horner_polynomial((T)9.0 - xAbs, polynomial4);

    const abacus_double polynomial5[16] = {
        0.11070463773286169990372436653511241845044712161429e0,
        0.21332789764918014983909802082047138329268827765885e-1,
        0.40406889155977461772988376159415397296440148973122e-2,
        0.75289681329516234268578523486963895653952058904406e-3,
        0.13810238588175738589710088734001335448824482725804e-3,
        0.24953881304742170109859300308574839972526234753948e-4,
        0.44444041661973769601354026252065956015993766811811e-5,
        0.78063899223184462500750715405536321985984924668052e-6,
        0.13522568513724882161932547902505233332689433323226e-6,
        0.23139264626715452923838613250310360877881259686444e-7,
        0.39468651950006128363940148642967408747660811317076e-8,
        0.65772668877031875276218754307071510803213337349400e-9,
        0.97560690416780322683931461761432410626330435434239e-10,
        0.16123134233870904955480002695710261854618012594367e-10,
        0.42935234747721987279511521489017102306945787381872e-11,
        0.67103905111008702565383310413536484200906759877260e-12};

    const T s5 =
        abacus::internal::horner_polynomial((T)5.0 - xAbs, polynomial5);

    // default to the first interval (xAbs <= 27)
    T result = s0;

    result = __abacus_select(result, s1, (SignedType)(xAbs <= 23.0));
    result = __abacus_select(result, s2, (SignedType)(xAbs <= 19.0));
    result = __abacus_select(result, s3, (SignedType)(xAbs <= 15.0));
    result = __abacus_select(result, s4, (SignedType)(xAbs <= 11.0));
    result = __abacus_select(result, s5, (SignedType)(xAbs <= 7.0));

    T xy_lo;
    const T xy_hi =
        abacus::internal::multiply_exact_unsafe(xAbs, -xAbs, &xy_lo);

    result *= abacus::internal::exp_unsafe(xy_lo) *
              abacus::internal::exp_unsafe(xy_hi);

    const abacus_double polynomial6[18] = {
        -0.268247063230831878723381994598e1,
        -0.867845086843047705372351110921e0,
        -0.279528774903648722426916321813e-1,
        0.574387967671668929992112078114e-2,
        -0.108656166561692131000126421105e-2,
        0.177081139908860557919195115743e-3,
        -0.208859000913560570921323998782e-4,
        0.897352722498382076513836604613e-7,
        0.931222471532178523344317808894e-6,
        -0.363800406195910372638643163471e-6,
        0.953193327966076924424170386519e-7,
        -0.187646922187368191589192252893e-7,
        0.241427046115354728660918457192e-8,
        0.324300370006200443579616274299e-10,
        -0.130720788104569722765550475568e-9,
        0.428625250812943653519791711301e-10,
        -0.769303783427077728603115605154e-11,
        0.649208507303079943817283734212e-12};

    const T s6 = x * abacus::internal::horner_polynomial(x - 2.0, polynomial6);

    const abacus_double polynomial7[18] = {
        -0.184960550993324824857621355148e1,
        -0.789362004301543011766107752042e0,
        -0.537452520569423488043582046660e-1,
        0.121841540104189473125630218788e-1,
        -0.215032857613822740931059503745e-2,
        0.172425555253169452429548436289e-3,
        0.648991508681480233562559268370e-4,
        -0.386287712171937008056450312476e-4,
        0.114876335027520512309253971723e-4,
        -0.191015797734888113596409141661e-5,
        -0.109013611726562791598176304619e-6,
        0.213011983301223088208510837258e-6,
        -0.881864091637685067787037119190e-7,
        0.226394957529925810435000597890e-7,
        -0.348606461446349743922106636278e-8,
        0.112561502377352460450281963620e-9,
        0.719978845989106022397954580379e-10,
        -0.112483613328048316890929995550e-10};

    const T s7 = x * abacus::internal::horner_polynomial(x - 1.0, polynomial7);

    const abacus_double polynomial8[18] = {-0.112837916709551257387779218929e1,
                                           -0.636619772367581355079779256278e0,
                                           -0.102772603301938669575917595813e0,
                                           0.191284470089803681405324878785e-1,
                                           0.209194641587716481495937472241e-3,
                                           -0.169620594916884365301150272231e-2,
                                           0.590123217041720813071506456586e-3,
                                           -0.258700639317410231340630725796e-4,
                                           -0.645135822185522141413097038931e-4,
                                           0.297617092364374087385518407456e-4,
                                           -0.344658303449755554224885623212e-5,
                                           -0.282844044768991465735295736011e-5,
                                           0.179871663428911307478704657569e-5,
                                           -0.452444188297404063112453647012e-6,
                                           -0.6183213537227261470178499653e-8,
                                           0.423728550087825528947547380634e-7,
                                           -0.130665471764135727617959508462e-7,
                                           0.147819914464175590200096454488e-8};

    const T s8 = x * abacus::internal::horner_polynomial(x, polynomial8);

    result = __abacus_select(result, s6, (SignedType)(xAbs <= 3.0));
    result = __abacus_select(result, s7, (SignedType)(xAbs <= 2.0));
    result = __abacus_select(result, s8, (SignedType)(xAbs <= 1.0));

    result = __abacus_select(result, abacus::internal::exp_unsafe(result),
                             (SignedType)(xAbs <= 3.0));

    result = __abacus_select(result, (T)2.0 - result,
                             (SignedType)__abacus_signbit(result));

    result = __abacus_select(result, (T)2.0, (SignedType)(x < -5.8));
    result = __abacus_select(result, (T)0.0, (SignedType)(x > 27.0));
    result =
        __abacus_select(result, (T)ABACUS_NAN, (SignedType)__abacus_isnan(x));

    return result;
  }
};
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT

template <typename T>
inline T erfc(const T x) {
  return erfc_helper<T>::_(x);
}
}  // namespace

#ifdef __CA_BUILTINS_HALF_SUPPORT
abacus_half ABACUS_API __abacus_erfc(abacus_half x) { return erfc<>(x); }
abacus_half2 ABACUS_API __abacus_erfc(abacus_half2 x) { return erfc<>(x); }
abacus_half3 ABACUS_API __abacus_erfc(abacus_half3 x) { return erfc<>(x); }
abacus_half4 ABACUS_API __abacus_erfc(abacus_half4 x) { return erfc<>(x); }
abacus_half8 ABACUS_API __abacus_erfc(abacus_half8 x) { return erfc<>(x); }
abacus_half16 ABACUS_API __abacus_erfc(abacus_half16 x) { return erfc<>(x); }
#endif  // __CA_BUILTINS_HALF_SUPPORT

abacus_float ABACUS_API __abacus_erfc(abacus_float x) { return erfc<>(x); }
abacus_float2 ABACUS_API __abacus_erfc(abacus_float2 x) { return erfc<>(x); }
abacus_float3 ABACUS_API __abacus_erfc(abacus_float3 x) { return erfc<>(x); }
abacus_float4 ABACUS_API __abacus_erfc(abacus_float4 x) { return erfc<>(x); }
abacus_float8 ABACUS_API __abacus_erfc(abacus_float8 x) { return erfc<>(x); }
abacus_float16 ABACUS_API __abacus_erfc(abacus_float16 x) { return erfc<>(x); }

#ifdef __CA_BUILTINS_DOUBLE_SUPPORT
abacus_double ABACUS_API __abacus_erfc(abacus_double x) { return erfc<>(x); }
abacus_double2 ABACUS_API __abacus_erfc(abacus_double2 x) { return erfc<>(x); }
abacus_double3 ABACUS_API __abacus_erfc(abacus_double3 x) { return erfc<>(x); }
abacus_double4 ABACUS_API __abacus_erfc(abacus_double4 x) { return erfc<>(x); }
abacus_double8 ABACUS_API __abacus_erfc(abacus_double8 x) { return erfc<>(x); }
abacus_double16 ABACUS_API __abacus_erfc(abacus_double16 x) {
  return erfc<>(x);
}
#endif  // __CA_BUILTINS_DOUBLE_SUPPORT
