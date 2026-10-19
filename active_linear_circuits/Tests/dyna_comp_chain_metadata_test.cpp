// Chain metadata regression for Dyna Comp (DS-1-style white-box integration).

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "extensions/white_box/pedals/dyna_comp_f32.h"
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
    const char* chain = nx_dyna_comp_get_chain_hash();
    expect_hex64(chain);
    expect(std::strcmp(chain, nx_white_box_composite_chain_hash_hex("dyna_comp")) == 0,
           "chain_hash matches catalog lookup");

    nx_white_box_composite_policy_t policy{};
    expect(nx_dyna_comp_get_composite_policy(&policy), "composite policy");
    expect(policy.backbone_fs == 96000U, "backbone_fs");
    expect(policy.rom_bake_tag != nullptr && std::strcmp(policy.rom_bake_tag, "balanced@96000") == 0,
           "rom_bake_tag");

    const size_t runtime_count = nx_dyna_comp_runtime_stage_count();
    expect(runtime_count == nx_white_box_composite_stage_count("dyna_comp"), "stage count");
    expect(runtime_count == static_cast<size_t>(NX_DYNA_COMP_MODULE_COUNT), "module count");

    static const char* k_ops[] = {"bjt_follower_out", "ota_comp", "rc_level"};
    for (size_t i = 0; i < runtime_count; ++i)
    {
        nx_white_box_composite_stage_info_t stage{};
        expect(nx_white_box_composite_stage_info_at("dyna_comp", i, &stage), "stage info");
        expect(std::strcmp(nx_dyna_comp_runtime_stage_id(i), stage.topology_id) == 0, "topology_id");
        expect(std::strcmp(nx_dyna_comp_module_id_string(static_cast<nx_dyna_comp_module_id>(i)),
                           stage.topology_id) == 0,
               "module_id_string");
        expect(std::strcmp(k_ops[i], stage.operator_key) == 0, "operator_key");
    }

    expect(std::strcmp(nx_dyna_comp_get_stage_spec_hash_hex(NX_DYNA_COMP_MODULE_COMPRESSOR),
                       nx_operator_spec_hash_hex("ota_comp")) == 0,
           "compressor stage spec_hash");

    std::fprintf(stderr, "dyna_comp_chain_metadata_test: ok\n");
    return 0;
}
