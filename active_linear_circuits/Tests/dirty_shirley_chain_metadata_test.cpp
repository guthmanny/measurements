// Chain metadata regression for Dirty Shirley (DS-1-style white-box integration).

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "extensions/white_box/pedals/dirty_shirley_f32.h"
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
    const char* chain = nx_dirty_shirley_get_chain_hash();
    expect_hex64(chain);
    expect(std::strcmp(chain, nx_white_box_composite_chain_hash_hex("dirty_shirley")) == 0,
           "chain_hash matches catalog lookup");

    nx_white_box_composite_policy_t policy{};
    expect(nx_dirty_shirley_get_composite_policy(&policy), "composite policy");
    expect(policy.backbone_fs == 96000U, "backbone_fs");
    expect(policy.rom_bake_tag != nullptr && std::strcmp(policy.rom_bake_tag, "balanced@96000") == 0,
           "rom_bake_tag");

    const size_t runtime_count = nx_dirty_shirley_runtime_stage_count();
    expect(runtime_count == nx_white_box_composite_stage_count("dirty_shirley"), "stage count");
    expect(runtime_count == static_cast<size_t>(NX_DIRTY_SHIRLEY_MODULE_COUNT), "module count");

    static const char* k_ops[] = {"beod_preamp", "dirty_shirley_tight", "beod_pregain", "beod_postgain",
                                  "beod_trim", "beod_clipper", "beod_bass", "dirty_shirley_mid"};
    for (size_t i = 0; i < runtime_count; ++i)
    {
        nx_white_box_composite_stage_info_t stage{};
        expect(nx_white_box_composite_stage_info_at("dirty_shirley", i, &stage), "stage info");
        expect(std::strcmp(nx_dirty_shirley_runtime_stage_id(i), stage.topology_id) == 0, "topology_id");
        expect(std::strcmp(nx_dirty_shirley_module_id_string(static_cast<nx_dirty_shirley_module_id>(i)),
                           stage.topology_id) == 0,
               "module_id_string");
        expect(std::strcmp(k_ops[i], stage.operator_key) == 0, "operator_key");
    }

    expect(std::strcmp(nx_dirty_shirley_get_stage_spec_hash_hex(NX_DIRTY_SHIRLEY_MODULE_TIGHT),
                       nx_operator_spec_hash_hex("dirty_shirley_tight")) == 0,
           "tight stage spec_hash");
    expect(nx_operator_spec_revision("dirty_shirley_tight") == 1U, "dirty_shirley_tight spec_revision 1");

    expect(std::strcmp(nx_dirty_shirley_get_stage_spec_hash_hex(NX_DIRTY_SHIRLEY_MODULE_MID),
                       nx_operator_spec_hash_hex("dirty_shirley_mid")) == 0,
           "mid stage spec_hash");
    expect(nx_operator_spec_revision("dirty_shirley_mid") == 1U, "dirty_shirley_mid spec_revision 1");

    std::fprintf(stderr, "dirty_shirley_chain_metadata_test: ok\n");
    return 0;
}
