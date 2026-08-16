#include "BjtCurveMath.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <mutex>

extern "C"
{
#include "core/nonlinear_circuits/bjt_common_emitter/bjt_common_emitter_design.h"
#include "core/nonlinear_circuits/bjt_follower/bjt_follower_design.h"
#include "core/nonlinear_circuits/bjt_follower_out/bjt_follower_out_design.h"
#include "core/utilities/table_params.h"
}

namespace bjt_curve
{
namespace
{
constexpr double kSampleRate = 48000.0;

bool isPnpImpl(BjtModelKind model) noexcept
{
  switch (model)
  {
    case BjtModelKind::IdealPnp:
    case BjtModelKind::Pnp3906:
    case BjtModelKind::Ac128:
      return true;
    default:
      return false;
  }
}

// Unified model parameter lookup across NPN and PNP devices. The MuDSP
// nx_bjt_t and nx_bjt_pnp_t structs share an identical layout, so we can
// return a single nx_bjt_t view for both polarities.
const nx_bjt_t& modelParamsFor(BjtModelKind model) noexcept
{
  switch (model)
  {
    case BjtModelKind::Npn3904:
      return nx_bjt_npn_models[1];
    case BjtModelKind::Npn2222:
      return nx_bjt_npn_models[2];
    case BjtModelKind::IdealPnp:
      return *reinterpret_cast<const nx_bjt_t*>(&nx_bjt_pnp_models[0]);
    case BjtModelKind::Pnp3906:
      return *reinterpret_cast<const nx_bjt_t*>(&nx_bjt_pnp_models[1]);
    case BjtModelKind::Ac128:
      return *reinterpret_cast<const nx_bjt_t*>(&nx_bjt_pnp_models[2]);
    case BjtModelKind::IdealNpn:
    default:
      return nx_bjt_npn_models[0];
  }
}

// Ebers-Moll currents for a device of either polarity. PNP devices conduct
// with negative Vbe/Vbc, so we mirror the junction voltages before evaluating
// the (NPN-polarity) MuDSP EM kernel; the returned Ib/Ic magnitudes are then
// positive in the forward-active region.
void emCurrents(BjtModelKind model, const nx_bjt_t& bjt, double vbe, double vbc, double* ib, double* ic) noexcept
{
  const double isVal = static_cast<double>(bjt.Is);
  const double vtN = static_cast<double>(bjt.Vt) * static_cast<double>(bjt.N);
  const double bf = static_cast<double>(bjt.Bf);
  const double br = static_cast<double>(bjt.Br);
  const double vaf = static_cast<double>(bjt.VAF);
  const double var = static_cast<double>(bjt.VAR);
  if (isPnp(model))
  {
    nx_bjt_em_currents_f64(isVal, vtN, bf, br, vaf, var, -vbe, -vbc, ib, ic);
  }
  else
  {
    nx_bjt_em_currents_f64(isVal, vtN, bf, br, vaf, var, vbe, vbc, ib, ic);
  }
}

double clampExpArg(double arg) noexcept
{
  if (arg < -50.0) return -50.0;
  if (arg > 50.0) return 50.0;
  return arg;
}

double ibFromVbe(double isVal, double bf, double vt, double vbe) noexcept
{
  const double expArg = clampExpArg(vbe / vt);
  return (isVal / bf) * (std::exp(expArg) - 1.0);
}

double icFromVbe(double isVal, double vt, double vbe, double icMax) noexcept
{
  const double expArg = clampExpArg(vbe / vt);
  const double icRaw = isVal * (std::exp(expArg) - 1.0);
  if (icMax > 0.0 && icRaw > icMax) return icMax;
  return icRaw;
}

double solveVbeCommonEmitter(double p, const bjt_common_emitter_coeffs_t& c) noexcept
{
  double vbe = (p < 0.0) ? -5.0 : p;
  if (vbe > 0.8) vbe = 0.8;

  for (int it = 0; it < 50; ++it)
  {
    const double vceDc = c.vc_dc - c.ve_dc;
    const double vbc = vbe - vceDc;
    double ib = 0.0;
    double icRaw = 0.0;
    nx_bjt_em_currents_f64(c.bjt_Is, c.bjt_Vt, c.bf, c.br, c.vaf, c.var, vbe, vbc, &ib, &icRaw);
    const double ic = icRaw > c.ic_max ? c.ic_max : icRaw;
    double gPi = 0.0;
    double gM = 0.0;
    nx_bjt_em_derivs_dvbe_f64(c.bjt_Is, c.bjt_Vt, c.bf, c.br, c.vaf, c.var, vbe, vbc, &gPi, &gM);
    if (icRaw > c.ic_max) gM = 1.0e-12;
    const double f = vbe - p - c.K_Ib * ib - c.K_Ic * ic;
    if (std::abs(f) < 1.0e-12) break;

    const double dF = 1.0 - c.K_Ib * gPi - c.K_Ic * gM;
    if (std::abs(dF) < 1.0e-18) break;

    double delta = f / dF;
    const double maxStep = 2.0 * c.bjt_Vt;
    if (delta > maxStep)
      delta = maxStep;
    else if (delta < -maxStep)
      delta = -maxStep;
    vbe -= delta;
  }

  return vbe;
}

double solveVbeFollower(double p, const bjt_follower_coeffs_t& c) noexcept
{
  double vbe = p;
  if (vbe < -5.0) vbe = -5.0;
  if (vbe > 0.8) vbe = 0.8;

  for (int it = 0; it < 50; ++it)
  {
    const double expArg = clampExpArg(vbe / c.bjt_Vt);
    const double expVbe = std::exp(expArg);
    const double ib = (c.bjt_Is / c.bf) * (expVbe - 1.0);
    const double f = vbe - p - c.Req * ib;
    if (std::abs(f) < 1.0e-12) break;

    const double dF = 1.0 - c.Req * (c.bjt_Is / (c.bf * c.bjt_Vt)) * expVbe;
    if (std::abs(dF) < 1.0e-18) break;

    double delta = f / dF;
    const double maxStep = 2.0 * c.bjt_Vt;
    if (delta > maxStep)
      delta = maxStep;
    else if (delta < -maxStep)
      delta = -maxStep;
    vbe -= delta;
  }

  return vbe;
}

double solveVbeFollowerOut(double p, const bjt_follower_out_coeffs_t& c) noexcept
{
  double vbe = p;
  if (vbe < -5.0) vbe = -5.0;
  if (vbe > 0.8) vbe = 0.8;

  for (int it = 0; it < 50; ++it)
  {
    const double expArg = clampExpArg(vbe / c.bjt_Vt);
    const double expVbe = std::exp(expArg);
    const double ib = (c.bjt_Is / c.bf) * (expVbe - 1.0);
    const double f = vbe - p - c.Req * ib;
    if (std::abs(f) < 1.0e-12) break;

    const double dF = 1.0 - c.Req * (c.bjt_Is / (c.bf * c.bjt_Vt)) * expVbe;
    if (std::abs(dF) < 1.0e-18) break;

    double delta = f / dF;
    const double maxStep = 2.0 * c.bjt_Vt;
    if (delta > maxStep)
      delta = maxStep;
    else if (delta < -maxStep)
      delta = -maxStep;
    vbe -= delta;
  }

  return vbe;
}

double ibAtCommonEmitterDc(const bjt_common_emitter_coeffs_t& c) noexcept
{
  const double vbe = c.vb_dc - c.ve_dc;
  const double vbc = c.vb_dc - c.vc_dc;
  double ib = 0.0;
  double icDummy = 0.0;
  nx_bjt_em_currents_f64(c.bjt_Is, c.bjt_Vt, c.bf, c.br, c.vaf, c.var, vbe, vbc, &ib, &icDummy);
  return ib;
}

double icAtCommonEmitterDc(const bjt_common_emitter_coeffs_t& c) noexcept
{
  const double vbe = c.vb_dc - c.ve_dc;
  const double vbc = c.vb_dc - c.vc_dc;
  double ibDummy = 0.0;
  double ic = 0.0;
  nx_bjt_em_currents_f64(c.bjt_Is, c.bjt_Vt, c.bf, c.br, c.vaf, c.var, vbe, vbc, &ibDummy, &ic);
  if (c.ic_max > 0.0 && ic > c.ic_max) return c.ic_max;
  return ic;
}
double findCommonEmitterR2ForIb(nx_bjt_common_emitter_config_t config, double targetIb) noexcept
{
  if (targetIb <= 0.0) return config.R2;

  double lo = 1000.0;
  double hi = 10.0e6;
  for (int i = 0; i < 48; ++i)
  {
    config.R2 = 0.5 * (lo + hi);
    bjt_common_emitter_sanitize_config(&config);
    const bjt_common_emitter_coeffs_t c = bjt_common_emitter_design_core(kSampleRate, &config);
    if (ibAtCommonEmitterDc(c) < targetIb)
      hi = config.R2;
    else
      lo = config.R2;
  }
  return 0.5 * (lo + hi);
}

bool isCommonEmitterDcValid(const nx_bjt_common_emitter_config_t& config, const bjt_common_emitter_coeffs_t& c) noexcept
{
  if (!std::isfinite(c.vb_dc) || !std::isfinite(c.vc_dc) || !std::isfinite(c.ve_dc)) return false;
  const double vbe = c.vb_dc - c.ve_dc;
  const double vce = c.vc_dc - c.ve_dc;
  if (!std::isfinite(vbe) || !std::isfinite(vce)) return false;
  if (vbe < -0.5 || vbe > 1.2) return false;
  if (vce < -0.2 || vce > config.vcc + 0.5) return false;
  return true;
}

DcOperatingPoint fillOperatingPoint(double vbe, double vce, double ib, double ic) noexcept
{
  DcOperatingPoint q;
  if (!std::isfinite(vbe) || !std::isfinite(vce) || !std::isfinite(ib) || !std::isfinite(ic)) return q;
  if (ib <= 0.0 && ic <= 0.0) return q;

  q.valid = true;
  // Vbe/Vce keep their sign (PNP devices operate with negative Vbe/Vce).
  q.vbe = static_cast<float>(vbe);
  q.vce = static_cast<float>(vce);
  q.ib = static_cast<float>(juce::jmax(0.0, ib));
  q.ic = static_cast<float>(juce::jmax(0.0, ic));
  return q;
}

struct TabulateSamples
{
  std::vector<double> vbe;
  std::vector<double> ib;
  std::vector<double> ic;
};

size_t usableTableEndIndex(const nl_circuits_table_params_f32_t& tableParams) noexcept
{
  return static_cast<size_t>(tableParams.coarse_neg_size + tableParams.fine_table_size);
}

void appendTableSample(TabulateSamples& samples, double vbe, double ib, double ic) noexcept
{
  samples.vbe.push_back(vbe);
  samples.ib.push_back(ib);
  samples.ic.push_back(ic);
}

void sortSamplesByVbe(TabulateSamples& samples)
{
  const size_t n = samples.vbe.size();
  if (n < 2) return;

  std::vector<size_t> order(n);
  for (size_t i = 0; i < n; ++i) order[i] = i;

  std::sort(order.begin(), order.end(), [&samples](size_t a, size_t b) { return samples.vbe[a] < samples.vbe[b]; });

  TabulateSamples sorted;
  sorted.vbe.reserve(n);
  sorted.ib.reserve(n);
  sorted.ic.reserve(n);
  for (const size_t idx : order)
  {
    sorted.vbe.push_back(samples.vbe[idx]);
    sorted.ib.push_back(samples.ib[idx]);
    sorted.ic.push_back(samples.ic[idx]);
  }
  samples = std::move(sorted);
}

double interpolate1D(const std::vector<double>& xs, const std::vector<double>& ys, double x)
{
  if (xs.empty() || ys.empty()) return 0.0;
  if (x <= xs.front()) return ys.front();
  if (x >= xs.back()) return ys.back();

  const auto it = std::upper_bound(xs.begin(), xs.end(), x);
  const size_t idx1 = static_cast<size_t>(it - xs.begin());
  const size_t idx0 = idx1 - 1;
  const double x0 = xs[idx0];
  const double x1 = xs[idx1];
  const double alpha = (x - x0) / juce::jmax(1.0e-12, x1 - x0);
  return ys[idx0] * (1.0 - alpha) + ys[idx1] * alpha;
}

double conductanceFromResistor(double ohms) noexcept
{
  if (ohms <= 0.0 || !std::isfinite(ohms)) return 0.0;
  return 1.0 / ohms;
}

struct DcLoadLineCoeffs
{
  double slope{0.0};
  double icIntercept{0.0};
};

// f1 KCL with fixed vb, Ib: Ic = icIntercept - slope * Vce (design_core G2/G4/G5 network).
DcLoadLineCoeffs computeLoadLineFromDesignCore(const nx_bjt_common_emitter_config_t& config, double vb,
                                               double ib) noexcept
{
  const double G2 = conductanceFromResistor(config.R2);
  const double G4 = conductanceFromResistor(config.R4);
  const double G5 = conductanceFromResistor(config.R5);
  const double K = G5 + G2;
  const double g4Safe = juce::jmax(G4, 1.0e-18);
  const double denom = 1.0 + K / g4Safe;
  const double slope = K / juce::jmax(denom, 1.0e-18);
  const double icIntercept = (G5 * config.vcc + G2 * vb - K * ib / g4Safe) / juce::jmax(denom, 1.0e-18);
  return {slope, icIntercept};
}

double loadLineIcAtVce(const DcLoadLineCoeffs& line, double vce) noexcept
{
  return juce::jmax(0.0, line.icIntercept - line.slope * vce);
}

bool commonEmitterFixedConfig(BjtModelKind model, nx_bjt_common_emitter_config_t& configOut) noexcept
{
  nx_bjt_common_emitter_config_init(&configOut);
  // The MuDSP CE core is NPN-polarity only. For PNP devices we keep the core in
  // NPN mode with a silicon NPN model so the bias network solves cleanly; the
  // caller mirrors the resulting DC voltages back to PNP polarity and evaluates
  // the PNP Ebers-Moll currents itself.
  configOut.bjt = NX_BJT_2N3904;
  configOut.custom_bjt = nullptr;
  bjt_common_emitter_sanitize_config(&configOut);
  return true;
}

std::vector<std::pair<float, float>> buildLoadLineSegment(const DcLoadLineCoeffs& line, double vceMin,
                                                          double vceMax) noexcept
{
  std::vector<std::pair<float, float>> segment;
  if (line.slope <= 0.0 || vceMax <= vceMin) return segment;

  const auto icAt = [&line](double vce) { return loadLineIcAtVce(line, vce); };

  double startVce = vceMin;
  double endVce = vceMax;

  if (line.icIntercept > 0.0)
  {
    const double vceCutoff = line.icIntercept / line.slope;
    endVce = juce::jmin(endVce, vceCutoff);
  }

  if (endVce <= startVce) return segment;

  const double startIc = icAt(startVce);
  const double endIc = icAt(endVce);
  if (startIc <= 0.0 && endIc <= 0.0) return segment;

  segment.emplace_back(static_cast<float>(startVce), static_cast<float>(startIc));
  segment.emplace_back(static_cast<float>(endVce), static_cast<float>(endIc));
  return segment;
}

TabulateSamples tabulateCommonEmitter(BjtModelKind model)
{
  TabulateSamples samples;
  nx_bjt_common_emitter_config_t config{};
  if (!commonEmitterFixedConfig(model, config)) return samples;

  nl_circuits_table_params_f32_t tableParams{};
  nl_circuits_cache_f32_t cache{};
  std::memset(&cache, 0, sizeof(cache));
  cache.tables_num = 1;

  if (bjt_common_emitter_tabulate_f32(&tableParams, kSampleRate, &config, &cache) != NX_SUCCESS ||
      cache.tables == nullptr || cache.tables[0] == nullptr)
  {
    nl_circuits_cache_release_f32(&cache);
    return samples;
  }

  const bjt_common_emitter_coeffs_t coeffs = bjt_common_emitter_design_core(kSampleRate, &config);
  const size_t tableEnd = usableTableEndIndex(tableParams);

  for (size_t j = 0; j < tableEnd; ++j)
  {
    const double p = bjt_common_emitter_table_p_at_index_f32(&tableParams, j);
    const double ib = static_cast<double>(cache.tables[0][j]);
    const double vbe = solveVbeCommonEmitter(p, coeffs);
    const double vceDc = coeffs.vc_dc - coeffs.ve_dc;
    const double vbc = vbe - vceDc;
    double ibEm = 0.0;
    double ic = 0.0;
    nx_bjt_em_currents_f64(coeffs.bjt_Is, coeffs.bjt_Vt, coeffs.bf, coeffs.br, coeffs.vaf, coeffs.var, vbe, vbc, &ibEm,
                           &ic);
    if (coeffs.ic_max > 0.0 && ic > coeffs.ic_max) ic = coeffs.ic_max;
    appendTableSample(samples, vbe, ib, ic);
  }

  sortSamplesByVbe(samples);

  nl_circuits_cache_release_f32(&cache);
  return samples;
}

TabulateSamples tabulateFollower(BjtModelKind model)
{
  TabulateSamples samples;
  nx_bjt_follower_config_t config{};
  nx_bjt_follower_config_init(&config);
  config.bjt = isPnp(model) ? NX_BJT_2N3904 : static_cast<nx_bjt_npn_model_e>(static_cast<uint32_t>(model) & 0xFFU);
  if (isPnp(model)) config.custom_bjt = &modelParamsFor(model);

  nl_circuits_table_params_f32_t tableParams{};
  nl_circuits_cache_f32_t cache{};
  std::memset(&cache, 0, sizeof(cache));
  cache.tables_num = 1;

  if (bjt_follower_tabulate_f32(&tableParams, kSampleRate, &config, &cache) != NX_SUCCESS || cache.tables == nullptr ||
      cache.tables[0] == nullptr)
  {
    nl_circuits_cache_release_f32(&cache);
    return samples;
  }

  const bjt_follower_coeffs_t coeffs = bjt_follower_design_core(kSampleRate, &config);
  const size_t tableEnd = usableTableEndIndex(tableParams);

  for (size_t j = 0; j < tableEnd; ++j)
  {
    const double p = bjt_follower_table_p_at_index_f32(&tableParams, j);
    const double ib = static_cast<double>(cache.tables[0][j]);
    const double vbe = solveVbeFollower(p, coeffs);
    const double ic = icFromVbe(coeffs.bjt_Is, coeffs.bjt_Vt, vbe, -1.0);
    appendTableSample(samples, vbe, ib, ic);
  }

  sortSamplesByVbe(samples);

  nl_circuits_cache_release_f32(&cache);
  return samples;
}

TabulateSamples tabulateFollowerOut(BjtModelKind model)
{
  TabulateSamples samples;
  nx_bjt_follower_out_config_t config{};
  nx_bjt_follower_out_config_init(&config);
  config.bjt = isPnp(model) ? NX_BJT_2N3904 : static_cast<nx_bjt_npn_model_e>(static_cast<uint32_t>(model) & 0xFFU);
  if (isPnp(model)) config.custom_bjt = &modelParamsFor(model);

  nl_circuits_table_params_f32_t tableParams{};
  nl_circuits_cache_f32_t cache{};
  std::memset(&cache, 0, sizeof(cache));
  cache.tables_num = 1;

  if (bjt_follower_out_tabulate_f32(&tableParams, kSampleRate, &config, &cache) != NX_SUCCESS ||
      cache.tables == nullptr || cache.tables[0] == nullptr)
  {
    nl_circuits_cache_release_f32(&cache);
    return samples;
  }

  const bjt_follower_out_coeffs_t coeffs = bjt_follower_out_design_core(kSampleRate, &config);
  const size_t tableEnd = usableTableEndIndex(tableParams);

  for (size_t j = 0; j < tableEnd; ++j)
  {
    const double p = bjt_follower_out_table_p_at_index_f32(&tableParams, j);
    const double ib = static_cast<double>(cache.tables[0][j]);
    const double vbe = solveVbeFollowerOut(p, coeffs);
    const double ic = icFromVbe(coeffs.bjt_Is, coeffs.bjt_Vt, vbe, -1.0);
    appendTableSample(samples, vbe, ib, ic);
  }

  sortSamplesByVbe(samples);

  nl_circuits_cache_release_f32(&cache);
  return samples;
}

struct TabulateCacheEntry
{
  BjtModelKind model{BjtModelKind::IdealNpn};
  CircuitKind circuit{CircuitKind::CommonEmitter};
  TabulateSamples samples;
  bool valid{false};
};

struct FamilyCacheEntry
{
  BjtModelKind model{BjtModelKind::IdealNpn};
  CircuitKind circuit{CircuitKind::CommonEmitter};
  std::vector<float> ibValues;
  int numPoints{0};
  float vceMaxVolts{0.0f};
  std::vector<std::vector<std::pair<float, float>>> families;
  bool valid{false};
};

std::mutex& curveCacheMutex()
{
  static std::mutex mutex;
  return mutex;
}

TabulateCacheEntry& tabulateCache()
{
  static TabulateCacheEntry entry;
  return entry;
}

FamilyCacheEntry& familyCache()
{
  static FamilyCacheEntry entry;
  return entry;
}

bool ibValuesEqual(const std::vector<float>& a, const std::vector<float>& b) noexcept
{
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i)
  {
    if (!juce::approximatelyEqual(a[i], b[i])) return false;
  }
  return true;
}

TabulateSamples tabulateCircuit(BjtModelKind model, CircuitKind circuit)
{
  {
    std::lock_guard<std::mutex> lock(curveCacheMutex());
    auto& cache = tabulateCache();
    if (cache.valid && cache.model == model && cache.circuit == circuit) return cache.samples;
  }

  TabulateSamples samples;
  switch (circuit)
  {
    case CircuitKind::Follower:
      samples = tabulateFollower(model);
      break;
    case CircuitKind::FollowerOut:
      samples = tabulateFollowerOut(model);
      break;
    case CircuitKind::CommonEmitter:
    default:
      samples = tabulateCommonEmitter(model);
      break;
  }

  {
    std::lock_guard<std::mutex> lock(curveCacheMutex());
    auto& cache = tabulateCache();
    cache.model = model;
    cache.circuit = circuit;
    cache.samples = samples;
    cache.valid = true;
  }
  return samples;
}

// Device transfer curves from NuDSP EM (not the audio LUT).
// The LUT is indexed by control variable p; inverting p→Vbe near saturation is
// ill-conditioned and produces non-monotonic Ib (sawteeth) after sorting.
std::vector<std::pair<float, float>> buildTransferCurveFromEm(BjtModelKind model, CircuitKind circuit, CurveKind kind,
                                                              int numPoints, float vbeMaxVolts)
{
  std::vector<std::pair<float, float>> points;
  if (numPoints < 2 || kind == CurveKind::IcVsVce) return points;

  const nx_bjt_t& bjt = modelParamsFor(model);
  const double isVal = static_cast<double>(bjt.Is);
  const double vtN = static_cast<double>(bjt.Vt) * static_cast<double>(bjt.N);
  const double bf = static_cast<double>(bjt.Bf);
  const double br = static_cast<double>(bjt.Br);
  const double vaf = static_cast<double>(bjt.VAF);
  const double var = static_cast<double>(bjt.VAR);
  const bool pnp = isPnp(model);

  // Bias Vbc with the circuit's DC operating point so CE/Follower stay consistent
  // with design_core; keep Ir small in the forward-active region.
  double vceBias = 5.0;
  switch (circuit)
  {
    case CircuitKind::CommonEmitter:
    {
      nx_bjt_common_emitter_config_t config{};
      if (!commonEmitterFixedConfig(model, config)) return points;
      const auto c = bjt_common_emitter_design_core(kSampleRate, &config);
      vceBias = juce::jmax(0.5, c.vc_dc - c.ve_dc);
      break;
    }
    case CircuitKind::Follower:
    {
      nx_bjt_follower_config_t config{};
      nx_bjt_follower_config_init(&config);
      config.bjt = NX_BJT_2N3904;
      config.custom_bjt = nullptr;
      const auto c = bjt_follower_design_core(kSampleRate, &config);
      // Collector at Vcc, emitter at Ve ⇒ Vce ≈ Vcc − Ve.
      vceBias = juce::jmax(0.5, config.vcc - c.ve_dc);
      break;
    }
    case CircuitKind::FollowerOut:
    {
      nx_bjt_follower_out_config_t config{};
      nx_bjt_follower_out_config_init(&config);
      config.bjt = NX_BJT_2N3904;
      config.custom_bjt = nullptr;
      const auto c = bjt_follower_out_design_core(kSampleRate, &config);
      vceBias = juce::jmax(0.5, config.vcc - c.ve_dc);
      break;
    }
  }
  // PNP devices operate with negative Vce, so mirror the bias magnitude.
  if (pnp) vceBias = -vceBias;

  // X window from 0 to user Vbe max; cap below exp(Vbe/Vt) overflow.
  // PNP devices conduct with negative Vbe, so sweep the mirrored positive
  // magnitude and plot on the negative axis.
  const double minVbe = static_cast<double>(kDefaultMinVbe);
  const double maxVbe = juce::jlimit(0.20, 1.20, static_cast<double>(vbeMaxVolts));

  points.reserve(static_cast<size_t>(numPoints));
  for (int i = 0; i < numPoints; ++i)
  {
    const double t = static_cast<double>(i) / static_cast<double>(numPoints - 1);
    const double vbeMag = minVbe + t * (maxVbe - minVbe);
    const double vbe = pnp ? -vbeMag : vbeMag;
    const double vbc = vbe - vceBias;
    double ib = 0.0;
    double ic = 0.0;
    nx_bjt_em_currents_f64(isVal, vtN, bf, br, vaf, var, vbe, vbc, &ib, &ic);

    switch (kind)
    {
      case CurveKind::IcVsVbe:
        points.emplace_back(static_cast<float>(vbe), static_cast<float>(juce::jmax(0.0, ic)));
        break;
      case CurveKind::IbVsVbe:
        points.emplace_back(static_cast<float>(vbe), static_cast<float>(juce::jmax(0.0, ib)));
        break;
      case CurveKind::IcVsVce:
        break;
    }
  }

  return points;
}

// Constant-Ib device characteristic from NuDSP Ebers-Moll.
// For each Vce: solve Vbe so Ib(Vbe, Vbe−Vce)=targetIb, then Ic = EM(Vbe, Vbc).
// (CE network Vcc-sweep cannot reach deep saturation at fixed Ib — ohmic region vanishes.)
// For PNP devices the junction voltages are mirrored (negative), so we solve on
// the mirrored positive magnitudes and return the PNP-polarity Vbe.
double solveVbeForConstantIb(const nx_bjt_t& bjt, bool pnp, double targetIb, double vce) noexcept
{
  const double isVal = static_cast<double>(bjt.Is);
  const double vtN = static_cast<double>(bjt.Vt) * static_cast<double>(bjt.N);
  const double bf = static_cast<double>(bjt.Bf);
  const double br = static_cast<double>(bjt.Br);
  const double vaf = static_cast<double>(bjt.VAF);
  const double var = static_cast<double>(bjt.VAR);

  // Work in mirrored (positive) magnitudes for both polarities.
  const double vceMag = std::abs(vce);

  // Active-region seed: Ib ≈ (Is/Bf)*(exp(Vbe/Vt)-1)
  double vbeMag =
      vtN * std::log(1.0 + juce::jmax(1.0e-30, targetIb) * juce::jmax(bf, 1.0) / juce::jmax(isVal, 1.0e-30));
  vbeMag = juce::jlimit(0.0, 1.2, vbeMag);

  for (int it = 0; it < 60; ++it)
  {
    const double vbcMag = vbeMag - vceMag;
    double ib = 0.0;
    double ic = 0.0;
    nx_bjt_em_currents_f64(isVal, vtN, bf, br, vaf, var, vbeMag, vbcMag, &ib, &ic);

    const double f = ib - targetIb;
    if (std::abs(f) < 1.0e-15 * juce::jmax(1.0, targetIb) || std::abs(f) < 1.0e-18) break;

    double dibDvbe = 0.0;
    double dicDvbe = 0.0;
    double dibDvbc = 0.0;
    double dicDvbc = 0.0;
    nx_bjt_em_derivs_dvbe_f64(isVal, vtN, bf, br, vaf, var, vbeMag, vbcMag, &dibDvbe, &dicDvbe);
    nx_bjt_em_derivs_dvbc_f64(isVal, vtN, bf, br, vaf, var, vbeMag, vbcMag, &dibDvbc, &dicDvbc);
    // Vbc = Vbe − Vce ⇒ dIb/dVbe = ∂Ib/∂Vbe + ∂Ib/∂Vbc
    const double dF = dibDvbe + dibDvbc;
    if (std::abs(dF) < 1.0e-30) break;

    double delta = f / dF;
    const double maxStep = 4.0 * vtN;
    if (delta > maxStep)
      delta = maxStep;
    else if (delta < -maxStep)
      delta = -maxStep;
    vbeMag -= delta;
    vbeMag = juce::jlimit(0.0, 1.2, vbeMag);
  }

  return pnp ? -vbeMag : vbeMag;
}

std::vector<std::pair<float, float>> buildCommonEmitterOutputCurve(BjtModelKind model, double targetIb, int numPoints,
                                                                   float vceMaxVolts)
{
  std::vector<std::pair<float, float>> points;
  if (numPoints < 2 || targetIb <= 0.0) return points;

  const nx_bjt_t& bjt = modelParamsFor(model);
  const double isVal = static_cast<double>(bjt.Is);
  const double vtN = static_cast<double>(bjt.Vt) * static_cast<double>(bjt.N);
  const double bf = static_cast<double>(bjt.Bf);
  const double br = static_cast<double>(bjt.Br);
  const double vaf = static_cast<double>(bjt.VAF);
  const double var = static_cast<double>(bjt.VAR);
  const bool pnp = isPnp(model);
  const double vceMax = juce::jmax(0.05, static_cast<double>(vceMaxVolts));

  // Dense sampling in the soft-knee / ohmic region, coarser in the active region.
  const int kneePoints = juce::jmax(24, numPoints / 3);
  const int activePoints = juce::jmax(8, numPoints - kneePoints);
  const double kneeEnd = juce::jmin(1.2, vceMax * 0.35);

  points.reserve(static_cast<size_t>(kneePoints + activePoints));

  auto appendAtVce = [&](double vceMag)
  {
    vceMag = juce::jlimit(0.0, vceMax, vceMag);
    const double vbe = solveVbeForConstantIb(bjt, pnp, targetIb, vceMag);
    const double vbeMag = std::abs(vbe);
    const double vbcMag = vbeMag - vceMag;
    double ib = 0.0;
    double ic = 0.0;
    nx_bjt_em_currents_f64(isVal, vtN, bf, br, vaf, var, vbeMag, vbcMag, &ib, &ic);
    if (!std::isfinite(ic)) return;
    // Device curve: no circuit ic_max clamp — that belongs to the CE load line / Q-point.
    // PNP plots on the negative Vce axis.
    const double plotVce = pnp ? -vceMag : vceMag;
    points.emplace_back(static_cast<float>(plotVce), static_cast<float>(juce::jmax(0.0, ic)));
  };

  for (int i = 0; i < kneePoints; ++i)
  {
    const double t = static_cast<double>(i) / static_cast<double>(kneePoints - 1);
    // Quadratic spacing packs more points near Vce=0.
    appendAtVce(kneeEnd * t * t);
  }
  for (int i = 1; i < activePoints; ++i)
  {
    const double t = static_cast<double>(i) / static_cast<double>(activePoints - 1);
    appendAtVce(kneeEnd + t * (vceMax - kneeEnd));
  }

  if (points.size() < 2) return {};

  std::sort(points.begin(), points.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

  std::vector<std::pair<float, float>> unique;
  unique.reserve(points.size());
  unique.push_back(points.front());
  for (size_t i = 1; i < points.size(); ++i)
  {
    if (points[i].first <= unique.back().first + 1.0e-6f)
      unique.back() = points[i];
    else
      unique.push_back(points[i]);
  }
  return unique;
}

// True geometric intersection of load line with one Ic–Vce curve inside [0, vceMax].
// Returns ic==0 when there is no crossing in-range (do not invent a fake Q on the curve).
DcOperatingPoint intersectLoadLineWithCurve(const DcLoadLineCoeffs& line,
                                            const std::vector<std::pair<float, float>>& curve,
                                            float vceMaxVolts) noexcept
{
  DcOperatingPoint q{};
  if (curve.size() < 2 || line.slope <= 0.0) return q;

  const double vMax = static_cast<double>(vceMaxVolts);

  for (size_t i = 0; i + 1 < curve.size(); ++i)
  {
    double vce0 = curve[i].first;
    double vce1 = curve[i + 1].first;
    if (vce1 < 0.0 || vce0 > vMax) continue;

    double ic0 = curve[i].second;
    double ic1 = curve[i + 1].second;

    // Clip segment to [0, vMax] so edge intersections are not missed.
    if (vce0 < 0.0)
    {
      const double t = (0.0 - vce0) / juce::jmax(1.0e-18, vce1 - vce0);
      ic0 = ic0 + t * (ic1 - ic0);
      vce0 = 0.0;
    }
    if (vce1 > vMax)
    {
      const double t = (vMax - vce0) / juce::jmax(1.0e-18, vce1 - vce0);
      ic1 = ic0 + t * (ic1 - ic0);
      vce1 = vMax;
    }
    if (vce1 <= vce0) continue;

    const double d0 = loadLineIcAtVce(line, vce0) - ic0;
    const double d1 = loadLineIcAtVce(line, vce1) - ic1;
    if (d0 * d1 > 0.0) continue;

    const double denom = d1 - d0;
    const double alpha = std::abs(denom) > 1.0e-18 ? juce::jlimit(0.0, 1.0, -d0 / denom) : 0.5;
    const double vce = vce0 + alpha * (vce1 - vce0);
    const double ic = ic0 + alpha * (ic1 - ic0);
    if (vce >= 0.0 && vce <= vMax && ic > 0.0)
    {
      // Place Q exactly on the load line at this Vce (matches drawn dashed line).
      q.vce = static_cast<float>(vce);
      q.ic = static_cast<float>(loadLineIcAtVce(line, vce));
      return q;
    }
  }

  // Active-region closed form: Ic ≈ β·Ib (curve plateau) ∩ load line.
  const double icActive = static_cast<double>(curve.back().second);
  if (icActive > 0.0 && line.slope > 0.0)
  {
    const double vce = (line.icIntercept - icActive) / line.slope;
    if (vce >= 0.0 && vce <= vMax)
    {
      q.vce = static_cast<float>(vce);
      q.ic = static_cast<float>(loadLineIcAtVce(line, vce));
    }
  }

  return q;
}

}  // namespace

bool isPnp(BjtModelKind model) noexcept { return isPnpImpl(model); }

const nx_bjt_t& modelParams(BjtModelKind model) noexcept { return modelParamsFor(model); }

const char* modelDisplayName(BjtModelKind model) noexcept
{
  switch (model)
  {
    case BjtModelKind::Npn3904:
      return "2N3904";
    case BjtModelKind::Npn2222:
      return "2N2222";
    case BjtModelKind::IdealPnp:
      return "Ideal PNP";
    case BjtModelKind::Pnp3906:
      return "2N3906";
    case BjtModelKind::Ac128:
      return "AC128";
    case BjtModelKind::IdealNpn:
    default:
      return "Ideal NPN";
  }
}

const char* circuitDisplayName(CircuitKind circuit) noexcept
{
  switch (circuit)
  {
    case CircuitKind::Follower:
      return "BJT Follower";
    case CircuitKind::FollowerOut:
      return "BJT Follower Out";
    case CircuitKind::CommonEmitter:
    default:
      return "BJT Common Emitter";
  }
}

juce::String formatModelSummary(const nx_bjt_t& bjt) noexcept
{
  juce::String s = "Is=" + juce::String(bjt.Is, 2, true) + "  Bf=" + juce::String(bjt.Bf, 1) +
                   "  Vt=" + juce::String(bjt.Vt * 1000.0f, 1) + " mV";
  if (bjt.VAF > 0.0f) s += "  VAF=" + juce::String(bjt.VAF, 1) + " V";
  return s;
}

juce::String formatIbSweepSummary(const IbSweepParams& params)
{
  IbSweepParams safe = params;
  safe.sanitise();
  const float maxIb = safe.minAmps + safe.stepAmps * static_cast<float>(juce::jmax(0, safe.count - 1));
  return juce::String(safe.count) + " steps  |  Ib " + formatCurrentWithUnit(safe.minAmps) + " .. " +
         formatCurrentWithUnit(maxIb) + "  |  step " + formatCurrentWithUnit(safe.stepAmps);
}

std::vector<float> buildIbSweepValues(const IbSweepParams& params)
{
  IbSweepParams safe = params;
  safe.sanitise();

  std::vector<float> values;
  values.reserve(static_cast<size_t>(safe.count));
  for (int i = 0; i < safe.count; ++i) values.push_back(safe.minAmps + safe.stepAmps * static_cast<float>(i));

  return values;
}

std::vector<std::pair<float, float>> buildCurve(BjtModelKind model, CircuitKind circuit, CurveKind kind, float ibAmps,
                                                int numPoints, float vceMaxVolts, float vbeMaxVolts)
{
  if (kind == CurveKind::IcVsVce)
  {
    if (circuit == CircuitKind::CommonEmitter)
      return buildCommonEmitterOutputCurve(model, ibAmps, numPoints, vceMaxVolts);
    return {};
  }

  (void)ibAmps;
  (void)vceMaxVolts;
  return buildTransferCurveFromEm(model, circuit, kind, numPoints, vbeMaxVolts);
}

std::vector<std::vector<std::pair<float, float>>> buildIcVsVceFamily(BjtModelKind model, CircuitKind circuit,
                                                                     const std::vector<float>& ibValues, int numPoints,
                                                                     float vceMaxVolts)
{
  if (circuit != CircuitKind::CommonEmitter) return {};

  {
    std::lock_guard<std::mutex> lock(curveCacheMutex());
    auto& cache = familyCache();
    if (cache.valid && cache.model == model && cache.circuit == circuit && cache.numPoints == numPoints &&
        juce::approximatelyEqual(cache.vceMaxVolts, vceMaxVolts) && ibValuesEqual(cache.ibValues, ibValues))
    {
      return cache.families;
    }
  }

  std::vector<std::vector<std::pair<float, float>>> families;
  families.reserve(ibValues.size());
  for (const float ib : ibValues)
  {
    if (ib <= 0.0f) continue;
    families.push_back(buildCommonEmitterOutputCurve(model, static_cast<double>(ib), numPoints, vceMaxVolts));
  }

  {
    std::lock_guard<std::mutex> lock(curveCacheMutex());
    auto& cache = familyCache();
    cache.model = model;
    cache.circuit = circuit;
    cache.ibValues = ibValues;
    cache.numPoints = numPoints;
    cache.vceMaxVolts = vceMaxVolts;
    cache.families = families;
    cache.valid = true;
  }
  return families;
}

void curveAxisLabels(CurveKind kind, juce::String& xLabel, juce::String& yLabel)
{
  switch (kind)
  {
    case CurveKind::IcVsVbe:
      xLabel = "Vbe (V)";
      yLabel = "Ic";
      break;
    case CurveKind::IbVsVbe:
      xLabel = "Vbe (V)";
      yLabel = "Ib";
      break;
    case CurveKind::IcVsVce:
      xLabel = "Vce (V)";
      yLabel = "Ic";
      break;
  }
}

CurrentDisplayUnit chooseCurrentDisplayUnit(float maxAbsAmps) noexcept
{
  const float absAmps = std::abs(maxAbsAmps);
  if (absAmps >= 1.0e-3f) return CurrentDisplayUnit::Milliamps;
  if (absAmps >= 1.0e-6f) return CurrentDisplayUnit::Microamps;
  if (absAmps >= 1.0e-9f) return CurrentDisplayUnit::Nanoamps;
  return CurrentDisplayUnit::Amps;
}

juce::String currentAxisLabel(CurrentDisplayUnit unit, const juce::String& quantity) noexcept
{
  switch (unit)
  {
    case CurrentDisplayUnit::Milliamps:
      return quantity + " (mA)";
    case CurrentDisplayUnit::Microamps:
      return quantity + " (uA)";
    case CurrentDisplayUnit::Nanoamps:
      return quantity + " (nA)";
    case CurrentDisplayUnit::Amps:
    default:
      return quantity + " (A)";
  }
}

juce::String formatCurrentTick(float amps, CurrentDisplayUnit unit) noexcept
{
  switch (unit)
  {
    case CurrentDisplayUnit::Milliamps:
      return juce::String(amps * 1000.0f, std::abs(amps) >= 0.01f ? 1 : 2);
    case CurrentDisplayUnit::Microamps:
      return juce::String(amps * 1.0e6f, std::abs(amps) >= 1.0e-5f ? 1 : 2);
    case CurrentDisplayUnit::Nanoamps:
      return juce::String(amps * 1.0e9f, 1);
    case CurrentDisplayUnit::Amps:
    default:
      return juce::String(amps, 3);
  }
}

AxisRange computeAxisRange(CurveKind kind, float ibAmps, const std::vector<std::pair<float, float>>& samples)
{
  AxisRange range;
  if (samples.empty()) return range;

  range.minX = samples.front().first;
  range.maxX = samples.back().first;
  range.minY = samples.front().second;
  range.maxY = samples.front().second;

  for (const auto& sample : samples)
  {
    range.minX = juce::jmin(range.minX, sample.first);
    range.maxX = juce::jmax(range.maxX, sample.first);
    range.minY = juce::jmin(range.minY, sample.second);
    range.maxY = juce::jmax(range.maxY, sample.second);
  }

  range.minY = 0.0f;
  range.maxY = juce::jmax(range.maxY * 1.08f, 1.0e-9f);

  if (kind == CurveKind::IcVsVce) (void)ibAmps;

  if (range.maxX <= range.minX) range.maxX = range.minX + 1.0f;

  return range;
}

void applyTransferAxisLimits(AxisRange& range, CurveKind kind, float vbeMaxVolts, float currentMaxAmps,
                             const DcOperatingPoint& qPoint) noexcept
{
  range.minX = kDefaultMinVbe;
  range.maxX = juce::jmax(0.20f, vbeMaxVolts);
  range.minY = 0.0f;
  range.maxY = juce::jmax(1.0e-12f, currentMaxAmps);

  if (!qPoint.valid) return;

  if (qPoint.vbe >= 0.0f) range.maxX = juce::jmax(range.maxX, qPoint.vbe);

  const float qCurrent = kind == CurveKind::IbVsVbe ? qPoint.ib : qPoint.ic;
  if (qCurrent > 0.0f) range.maxY = juce::jmax(range.maxY, qCurrent);
}

juce::String formatOperatingPointLabel(const DcOperatingPoint& q, CurveKind kind)
{
  if (!q.valid) return {};

  switch (kind)
  {
    case CurveKind::IbVsVbe:
      return "Q  Vbe=" + formatVoltageTick(q.vbe) + "  Ib=" + formatCurrentWithUnit(q.ib);
    case CurveKind::IcVsVbe:
      return "Q  Vbe=" + formatVoltageTick(q.vbe) + "  Ic=" + formatCurrentWithUnit(q.ic);
    case CurveKind::IcVsVce:
    default:
      return "Q  Vce=" + formatVoltageTick(q.vce) + "  Ic=" + formatCurrentWithUnit(q.ic);
  }
}

DcOperatingPoint computeStaticOperatingPoint(BjtModelKind model, CircuitKind circuit)
{
  const bool pnp = isPnp(model);
  switch (circuit)
  {
    case CircuitKind::CommonEmitter:
    {
      nx_bjt_common_emitter_config_t config{};
      if (!commonEmitterFixedConfig(model, config)) return {};
      const auto c = bjt_common_emitter_design_core(kSampleRate, &config);
      if (!isCommonEmitterDcValid(config, c)) return {};
      if (pnp)
      {
        // The CE core is NPN-polarity. For PNP we mirror the collector/emitter
        // DC voltages (negative Vce) and re-solve Vbe with the PNP Ebers-Moll
        // model so the base current matches the bias network.
        const double vce = -(c.vc_dc - c.ve_dc);
        const double targetIb = ibAtCommonEmitterDc(c);
        const double vbe = solveVbeForConstantIb(modelParamsFor(model), true, targetIb, std::abs(vce));
        const double vbc = vbe - vce;
        double ib = 0.0;
        double ic = 0.0;
        emCurrents(model, modelParamsFor(model), vbe, vbc, &ib, &ic);
        return fillOperatingPoint(vbe, vce, ib, ic);
      }
      return fillOperatingPoint(c.vb_dc - c.ve_dc, c.vc_dc - c.ve_dc, ibAtCommonEmitterDc(c), icAtCommonEmitterDc(c));
    }
    case CircuitKind::Follower:
    {
      nx_bjt_follower_config_t config{};
      nx_bjt_follower_config_init(&config);
      config.bjt = NX_BJT_2N3904;
      config.custom_bjt = nullptr;
      bjt_follower_sanitize_config(&config);
      const auto c = bjt_follower_design_core(kSampleRate, &config);
      const double vbe = pnp ? -(c.vb_dc - c.ve_dc) : (c.vb_dc - c.ve_dc);
      const double vce = pnp ? -(config.vcc - c.ve_dc) : (config.vcc - c.ve_dc);
      const double vbc = vbe - vce;
      double ib = 0.0;
      double ic = 0.0;
      emCurrents(model, modelParamsFor(model), vbe, vbc, &ib, &ic);
      return fillOperatingPoint(vbe, vce, ib, ic);
    }
    case CircuitKind::FollowerOut:
    {
      nx_bjt_follower_out_config_t config{};
      nx_bjt_follower_out_config_init(&config);
      config.bjt = NX_BJT_2N3904;
      config.custom_bjt = nullptr;
      bjt_follower_out_sanitize_config(&config);
      const auto c = bjt_follower_out_design_core(kSampleRate, &config);
      const double vbe = pnp ? -(c.vb_dc - c.ve_dc) : (c.vb_dc - c.ve_dc);
      const double vce = pnp ? -(config.vcc - c.ve_dc) : (config.vcc - c.ve_dc);
      const double vbc = vbe - vce;
      double ib = 0.0;
      double ic = 0.0;
      emCurrents(model, modelParamsFor(model), vbe, vbc, &ib, &ic);
      return fillOperatingPoint(vbe, vce, ib, ic);
    }
  }

  return {};
}

AxisRange computeAxisRangeForFamily(const std::vector<std::vector<std::pair<float, float>>>& families)
{
  AxisRange range;
  bool hasPoint = false;

  for (const auto& family : families)
  {
    for (const auto& sample : family)
    {
      if (!hasPoint)
      {
        range.minX = range.maxX = sample.first;
        range.minY = range.maxY = sample.second;
        hasPoint = true;
      }
      else
      {
        range.minX = juce::jmin(range.minX, sample.first);
        range.maxX = juce::jmax(range.maxX, sample.first);
        range.minY = juce::jmin(range.minY, sample.second);
        range.maxY = juce::jmax(range.maxY, sample.second);
      }
    }
  }

  range.minX = 0.0f;
  range.minY = 0.0f;
  range.maxY = juce::jmax(range.maxY * 1.08f, 1.0e-9f);
  if (range.maxX <= range.minX) range.maxX = range.minX + 1.0f;

  return range;
}

void expandAxisRangeForOverlay(AxisRange& range, const DcLoadLineOverlay& overlay, float vceMaxVolts)
{
  const float curveMaxY = range.maxY;
  const auto& q = overlay.qPoint;
  if (q.valid && q.vce >= 0.0f && q.ic > 0.0f)
  {
    range.maxX = juce::jmax(range.maxX, q.vce);
    range.maxY = juce::jmax(range.maxY, q.ic);
  }

  range.minX = 0.0f;
  range.minY = 0.0f;
  range.maxX = juce::jmax(range.maxX, vceMaxVolts);
  range.maxY = juce::jmax(juce::jmax(curveMaxY, range.maxY) * 1.08f, 1.0e-9f);
}

DcLoadLineOverlay buildCommonEmitterDcLoadLineOverlay(BjtModelKind model, const std::vector<float>& ibValues,
                                                      const std::vector<std::vector<std::pair<float, float>>>& families,
                                                      float vceMaxVolts)
{
  DcLoadLineOverlay overlay;
  if (ibValues.empty()) return overlay;

  const bool pnp = isPnp(model);

  nx_bjt_common_emitter_config_t baseConfig{};
  if (!commonEmitterFixedConfig(model, baseConfig)) return overlay;

  const bjt_common_emitter_coeffs_t baseCoeffs = bjt_common_emitter_design_core(kSampleRate, &baseConfig);
  const auto line = computeLoadLineFromDesignCore(baseConfig, baseCoeffs.vb_dc, 0.0);

  float maxFamilyIc = 0.0f;
  for (const auto& family : families)
  {
    for (const auto& p : family) maxFamilyIc = juce::jmax(maxFamilyIc, p.second);
  }

  (void)maxFamilyIc;
  (void)ibValues;
  (void)families;

  // One static Q-point: design_core DC of the fixed CE bias network.
  // The CE core is NPN-polarity; mirror the DC voltages for PNP devices.
  float qVce = 0.0f;
  if (isCommonEmitterDcValid(baseConfig, baseCoeffs))
  {
    if (pnp)
    {
      const double vce = -(baseCoeffs.vc_dc - baseCoeffs.ve_dc);
      const double targetIb = ibAtCommonEmitterDc(baseCoeffs);
      const double vbe = solveVbeForConstantIb(modelParamsFor(model), true, targetIb, std::abs(vce));
      const double vbc = vbe - vce;
      double ib = 0.0;
      double ic = 0.0;
      emCurrents(model, modelParamsFor(model), vbe, vbc, &ib, &ic);
      auto q = fillOperatingPoint(vbe, vce, ib, ic);
      if (q.valid)
      {
        qVce = q.vce;
        q.ic = static_cast<float>(loadLineIcAtVce(line, std::abs(q.vce)));
        overlay.qPoint = q;
      }
    }
    else
    {
      auto q = fillOperatingPoint(baseCoeffs.vb_dc - baseCoeffs.ve_dc, baseCoeffs.vc_dc - baseCoeffs.ve_dc,
                                  ibAtCommonEmitterDc(baseCoeffs), icAtCommonEmitterDc(baseCoeffs));
      if (q.valid)
      {
        qVce = q.vce;
        q.ic = static_cast<float>(loadLineIcAtVce(line, q.vce));
        overlay.qPoint = q;
      }
    }
  }

  // Store the physical load line out to at least the sweep / Q Vce; the panel clips
  // it to the final axis rectangle so the left end can meet the top of the plot.
  // PNP plots on the negative Vce axis, so mirror the load-line segment.
  const double loadLineEndVce = juce::jmax(static_cast<double>(vceMaxVolts), static_cast<double>(std::abs(qVce)));
  overlay.loadLines.resize(1);
  auto segment = buildLoadLineSegment(line, 0.0, loadLineEndVce);
  if (pnp)
  {
    for (auto& pt : segment) pt.first = -pt.first;
  }
  overlay.loadLines[0] = std::move(segment);

  return overlay;
}

std::vector<std::pair<float, float>> clipSegmentToAxisRange(const std::vector<std::pair<float, float>>& segment,
                                                            const AxisRange& range)
{
  std::vector<std::pair<float, float>> out;
  if (segment.size() < 2) return out;

  float x0 = segment.front().first;
  float y0 = segment.front().second;
  float x1 = segment.back().first;
  float y1 = segment.back().second;

  const float xmin = range.minX;
  const float xmax = range.maxX;
  const float ymin = range.minY;
  const float ymax = range.maxY;
  if (xmax <= xmin || ymax <= ymin) return out;

  // Liang–Barsky clip of one segment to the axis rectangle.
  const float dx = x1 - x0;
  const float dy = y1 - y0;
  float u0 = 0.0f;
  float u1 = 1.0f;

  const auto clipTest = [&](float p, float q) -> bool
  {
    if (std::abs(p) < 1.0e-20f) return q >= 0.0f;
    const float r = q / p;
    if (p < 0.0f)
    {
      if (r > u1) return false;
      if (r > u0) u0 = r;
    }
    else
    {
      if (r < u0) return false;
      if (r < u1) u1 = r;
    }
    return true;
  };

  if (!clipTest(-dx, x0 - xmin) || !clipTest(dx, xmax - x0) || !clipTest(-dy, y0 - ymin) || !clipTest(dy, ymax - y0) ||
      u0 > u1)
    return out;

  out.emplace_back(x0 + u0 * dx, y0 + u0 * dy);
  out.emplace_back(x0 + u1 * dx, y0 + u1 * dy);
  return out;
}

void clipOverlayLoadLinesToAxis(DcLoadLineOverlay& overlay, const AxisRange& range)
{
  for (auto& segment : overlay.loadLines) segment = clipSegmentToAxisRange(segment, range);
}

std::vector<float> buildNiceTicks(float minVal, float maxVal, int targetCount)
{
  std::vector<float> ticks;
  const float span = maxVal - minVal;
  if (span <= 0.0f) return {minVal};

  const float roughStep = span / static_cast<float>(juce::jmax(1, targetCount - 1));
  const float magnitude = std::pow(10.0f, std::floor(std::log10(roughStep)));
  const float normStep = roughStep / magnitude;
  float niceNorm = 10.0f;
  if (normStep <= 1.0f)
    niceNorm = 1.0f;
  else if (normStep <= 2.0f)
    niceNorm = 2.0f;
  else if (normStep <= 5.0f)
    niceNorm = 5.0f;

  const float step = niceNorm * magnitude;
  ticks.push_back(minVal);
  for (float v = std::ceil((minVal + step * 0.001f) / step) * step; v < maxVal - step * 0.01f; v += step)
  {
    if (v > minVal + step * 0.001f) ticks.push_back(v);
  }
  if (ticks.back() < maxVal - step * 0.001f) ticks.push_back(maxVal);

  return ticks;
}

juce::String formatVoltageTick(float volts)
{
  if (std::abs(volts) < 0.01f) return juce::String(volts * 1000.0f, 0) + " mV";

  return juce::String(volts, 2) + " V";
}

juce::String formatCurrentTick(float amps)
{
  const float absAmps = std::abs(amps);
  if (absAmps >= 1.0e-3f) return formatCurrentTick(amps, CurrentDisplayUnit::Milliamps);
  if (absAmps >= 1.0e-6f) return formatCurrentTick(amps, CurrentDisplayUnit::Microamps);
  if (absAmps >= 1.0e-9f) return formatCurrentTick(amps, CurrentDisplayUnit::Nanoamps);
  return formatCurrentTick(amps, CurrentDisplayUnit::Amps);
}

juce::String formatCurrentWithUnit(float amps)
{
  const float absAmps = std::abs(amps);
  if (absAmps >= 1.0e-3f) return formatCurrentTick(amps, CurrentDisplayUnit::Milliamps) + " mA";
  if (absAmps >= 1.0e-6f) return formatCurrentTick(amps, CurrentDisplayUnit::Microamps) + " uA";
  if (absAmps >= 1.0e-9f) return formatCurrentTick(amps, CurrentDisplayUnit::Nanoamps) + " nA";

  return "0 A";
}

}  // namespace bjt_curve
