#pragma once

// Simple list of instruments (display names). First entry is "None".
static const char* INSTRUMENT_NAMES[] = {
    "None",
    "arturia_microfreak_deluge",
    "asm_hydrasynth_deluge",
    "audiothingies_micromonsta_2_deluge",
    "behringer_neutron_deluge",
    "korg_volca-beats_deluge",
    "korg_volca-fm_deluge",
    "korg_volca-keys_deluge",
    "novation_circuit_deluge",
    "polyend_acd_synth_deluge",
    "polyend_fat_synth_deluge",
    "polyend_grain_synth_deluge",
    "polyend_phz_synth_deluge",
    "polyend_pmd_synth_deluge",
    "polyend_vap_synth_deluge",
    "polyend_wavs_synth_deluge",
    "polyend_wtfm_synth_deluge",
};

static const int INSTRUMENT_COUNT = sizeof(INSTRUMENT_NAMES) / sizeof(INSTRUMENT_NAMES[0]);
