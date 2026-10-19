// Chain metadata regression for the ODR-1 white-box composite.

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "extensions/white_box/pedals/odr1_f32.h"
#include "nudsp/common/operator_spec_metadata.h"

static void expect(bool cond, const char* message) {
    if (!cond) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

static void expect_hex64(const char* hex) {
    expect(hex != nullptr, "null hash");
    expect(std::strlen(hex) == 64U, "hash length");
}

int main() {
    const char* chain = nx_odr1_get_chain_hash();
    expect_hex64(chain);
    expect(std::strcmp(chain, nx_white_box_composite_chain_hash_hex("odr1")) == 0,
           "chain_hash matches catalog lookup");

    nx_white_box_composite_policy_t policy{};
    expect(nx_odr1_get_composite_policy(&policy), "composite policy");
    expect(policy.backbone_fs == 96000U, "backbone_fs");
    expect(policy.rom_bake_tag != nullptr && std::strcmp(policy.rom_bake_tag, "balanced@96000") == 0,
           "rom_bake_tag");

    const size_t runtime_count = nx_odr1_runtime_stage_count();
    expect(runtime_count == nx_white_box_composite_stage_count("odr1"), "stage count");
    expect(runtime_count == static_cast<size_t>(NX_ODR1_MODULE_COUNT), "module count");

    static const char* k_ops[] = {
        "jfet_follower", "odr1_drive", "odr1_clipper", "odr1_tone", "odr1_postamp", "rc_level", "op_buffer",
    };
    expect(runtime_count == sizeof(k_ops) / sizeof(k_ops[0]), "operator list length");
    for (size_t i = 0; i < runtime_count; ++i) {
        nx_white_box_composite_stage_info_t stage{};
        expect(nx_white_box_composite_stage_info_at("odr1", i, &stage), "stage info");
        expect(std::strcmp(nx_odr1_runtime_stage_id(i), stage.topology_id) == 0, "topology_id");
        expect(std::strcmp(nx_odr1_module_id_string(static_cast<nx_odr1_module_id>(i)), stage.topology_id) == 0,
               "module id string");
        expect(std::strcmp(k_ops[i], stage.operator_key) == 0, "operator key");
    }

    std::fprintf(stderr, "odr1_chain_metadata_test: ok\n");
    return 0;
}
