#include "stdafx.h"
#include "dsp_bridge.h"

namespace {

static bool nameLooksLikeDsdProcessor(const char* name) {
    if (!name) return false;
    const std::string n = name;
    return _stricmp(n.c_str(), "DSD Processor") == 0 ||
        (n.find("DSD") != std::string::npos && n.find("Processor") != std::string::npos);
}

static uint64_t fnv1a(const uint8_t* p, size_t n) {
    uint64_t h = 14695981039346656037ull;
    for (size_t i = 0; i < n; ++i) { h ^= p[i]; h *= 1099511628211ull; }
    return h;
}

static void appendU32(std::vector<uint8_t>& out, uint32_t v) {
    for (unsigned i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
}

static uint32_t readU32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

}

bool DsdProcessorBridge::installed(pfc::string_base* versionOut) {
    return dsd_processor_installed(versionOut);
}

bool DsdProcessorBridge::find_entry(service_ptr_t<dsp_entry>& out) {
    service_enum_t<dsp_entry> e;
    dsp_entry::ptr ptr;
    while (e.next(ptr)) {
        pfc::string8 name;
        ptr->get_name(name);
        if (nameLooksLikeDsdProcessor(name)) { out = ptr; return true; }
    }
    return false;
}

bool DsdProcessorBridge::find_default_preset(dsp_preset_impl& out) {
    service_ptr_t<dsp_entry> entry;
    if (!find_entry(entry)) return false;
    return entry->get_default_preset(out);
}

bool DsdProcessorBridge::load_saved_preset(dsp_preset_impl& out) {
    auto blob = sacd_dlna_cfg::dsd_processor_preset.get();
    if (!blob.is_valid() || blob->size() < 24) return find_default_preset(out);
    const uint8_t* p = static_cast<const uint8_t*>(blob->data());
    if (memcmp(p, "DSP1", 4) != 0) return find_default_preset(out);
    const uint32_t dataSize = readU32(p + 20);
    if (dataSize > blob->size() - 24) return find_default_preset(out);
    GUID owner{};
    memcpy(&owner, p + 4, sizeof(owner));
    out.set_owner(owner);
    out.set_data(p + 24, dataSize);
    // Reject stale/deleted DSP owners instead of creating an invalid chain.
    if (!dsp_entry::g_dsp_exists(owner)) return find_default_preset(out);
    return true;
}

bool DsdProcessorBridge::save_preset(const dsp_preset& preset) {
    std::vector<uint8_t> blob;
    blob.reserve(24 + preset.get_data_size());
    blob.insert(blob.end(), {'D','S','P','1'});
    const auto owner = preset.get_owner();
    const uint8_t* ownerBytes = reinterpret_cast<const uint8_t*>(&owner);
    blob.insert(blob.end(), ownerBytes, ownerBytes + sizeof(owner));
    // Bytes 20..23 hold data size.
    appendU32(blob, static_cast<uint32_t>(preset.get_data_size()));
    const uint8_t* data = static_cast<const uint8_t*>(preset.get_data());
    if (data && preset.get_data_size()) blob.insert(blob.end(), data, data + preset.get_data_size());
    sacd_dlna_cfg::dsd_processor_preset.set(blob.data(), blob.size());
    return true;
}

bool DsdProcessorBridge::configure(HWND parent) {
    if (!installed()) return false;
    service_ptr_t<dsp_entry> entry;
    if (!find_entry(entry)) return false;
    if (!entry->have_config_popup()) return false;
    dsp_preset_impl preset;
    if (!load_saved_preset(preset)) return false;
    if (!dsp_entry::g_show_config_popup(preset, parent)) return false;
    return save_preset(preset);
}

std::string DsdProcessorBridge::preset_fingerprint() {
    dsp_preset_impl preset;
    if (!load_saved_preset(preset)) return "none";
    const GUID owner = preset.get_owner();
    std::vector<uint8_t> bytes(sizeof(GUID) + preset.get_data_size());
    memcpy(bytes.data(), &owner, sizeof(GUID));
    if (preset.get_data_size()) memcpy(bytes.data() + sizeof(GUID), preset.get_data(), preset.get_data_size());
    char hex[32]{};
    snprintf(hex, sizeof(hex), "%016llX", static_cast<unsigned long long>(fnv1a(bytes.data(), bytes.size())));
    return hex;
}

bool DsdProcessorBridge::process_chunk(dsp_manager& manager, const metadb_handle_ptr& track,
                                       const audio_chunk& input, std::vector<audio_chunk>& output,
                                       abort_callback& abort) {
    dsp_chunk_list_impl list;
    list.add_chunk(&input);
    manager.run(&list, track, 0, abort);
    bool had = false;
    for (t_size i = 0; i < list.get_count(); ++i) {
        auto* c = list.get_item(i);
        if (!c || c->is_empty()) continue;
        output.emplace_back();
        output.back().copy(*c);
        had = true;
    }
    return had;
}

bool DsdProcessorBridge::flush(dsp_manager& manager, const metadb_handle_ptr& track,
                               std::vector<audio_chunk>& output, abort_callback& abort) {
    dsp_chunk_list_impl list;
    manager.run(&list, track, dsp::FLUSH, abort);
    bool had = false;
    for (t_size i = 0; i < list.get_count(); ++i) {
        auto* c = list.get_item(i);
        if (!c || c->is_empty()) continue;
        output.emplace_back();
        output.back().copy(*c);
        had = true;
    }
    return had;
}
