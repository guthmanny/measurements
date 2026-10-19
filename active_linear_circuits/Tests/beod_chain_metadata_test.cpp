// Chain metadata regression for BEOD (DS-1-style white-box integration).

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "extensions/white_box/pedals/beod_f32.h"
#include "nudsp/common/operator_spec_metadata.h"

static void expect(bool cond, const char* message)
{
    if (!cond)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

static void expect_hex64(const char* hex)
{
    expect(hex != nullptr, "null hash");
    expect(std::strlen(hex) == 64U, "hash length");
}

int main()
{
    const char* chain = nx_beod_get_chain_hash();
    expect_hex64(chain);
    expect(std::strcmp(chain, nx_white_box_composite_chain_hash_hex("beod")) == 0,
           "chain_hash matches catalog lookup");

    nx_white_box_composite_policy_t policy{};
    expect(nx_beod_get_composite_policy(&policy), "composite policy");
    expect(policy.backbone_fs == 96000U, "backbone_fs");
    expect(policy.rom_bake_tag != nullptr && std::strcmp(policy.rom_bake_tag, "balanced@96000") == 0,
           "rom_bake_tag");

    const size_t runtime_count = nx_beod_runtime_stage_count();
    expect(runtime_count == nx_white_box_composite_stage_count("beod"), "stage count");
    expect(runtime_count == static_cast<size_t>(NX_BEOD_MODULE_COUNT), "module count");

    static const char* k_ops[] = {"beod_preamp", "beod_tight", "beod_pregain", "beod_postgain", "beod_trim",
                                  "beod_clipper", "beod_bass", "rc_level"};
    for (size_t i = 0; i < runtime_count; ++i)
    {
        nx_white_box_composite_stage_info_t stage{};
        expect(nx_white_box_composite_stage_info_at("beod", i, &stage), "stage info");
        expect(std::strcmp(nx_beod_runtime_stage_id(i), stage.topology_id) == 0, "topology_id");
        expect(std::strcmp(nx_beod_module_id_string(static_cast<nx_beod_module_id>(i)), stage.topology_id) == 0,
               "module_id_string");
        expect(std::strcmp(k_ops[i], stage.operator_key) == 0, "operator_key");
    }

    expect(std::strcmp(nx_beod_get_stage_spec_hash_hex(NX_BEOD_MODULE_TRIM),
                       nx_operator_spec_hash_hex("beod_trim")) == 0,
           "trim stage spec_hash");
    expect(nx_operator_spec_revision("beod_trim") == 2U, "beod_trim spec_revision 2");

    expect(std::strcmp(nx_beod_get_stage_spec_hash_hex(NX_BEOD_MODULE_CLIPPER),
                       nx_operator_spec_hash_hex("beod_clipper")) == 0,
           "clipper stage spec_hash");
    expect(nx_operator_spec_revision("beod_clipper") == 3U, "beod_clipper spec_revision 3");

    expect(std::strcmp(nx_beod_get_stage_spec_hash_hex(NX_BEOD_MODULE_BASS),
                       nx_operator_spec_hash_hex("beod_bass")) == 0,
           "bass stage spec_hash");
    expect(nx_operator_spec_revision("beod_bass") == 2U, "beod_bass spec_revision 2");

    std::fprintf(stderr, "beod_chain_metadata_test: ok\n");
    return 0;
}
