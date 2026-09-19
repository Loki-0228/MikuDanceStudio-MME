#pragma once

#include <cstddef>
#include <string>

namespace mikudancestudio::mdl { struct ModelRecord; struct MorphRecord; struct BoneRecord; }
namespace mikudancestudio::text_encoding {
// PMM has fixed byte fields and no declared code page. Keep legacy encodings
// when exact; use a UTF-8 BOM only for text no legacy code page can represent.
bool Decode(const char* bytes, unsigned int codePage, std::wstring& result);
bool EncodeExact(const wchar_t* text, unsigned int codePage, std::string& result);
bool EncodePath(char* destination, std::size_t capacity, const wchar_t* path);
bool EncodeDisplay(char* destination, std::size_t capacity, const wchar_t* text);
std::wstring Display(const char* text);
std::wstring LegacyFilename(const char* text, const wchar_t* resolvedPath);
bool HasUtf8Bom(const char* text);
std::wstring ModelName(const mdl::ModelRecord& model, bool english);
// UI identity comes from the resolved source file, not language-specific metadata.
std::wstring ModelLabel(const mdl::ModelRecord& model, bool english);
std::wstring MorphName(const mdl::MorphRecord& morph, bool english);
std::wstring BoneName(const mdl::BoneRecord& bone, bool english);
// Message boxes whose text comes from the byte-exact Shift-JIS literals of the
// original image (the JP branches) or from plain ASCII (the EN branches).
// MessageBoxA would decode those bytes with the *system* code page and garble
// every Japanese line on a non-Japanese Windows, so decode as CP932 and call
// MessageBoxW.  owner is an HWND (kept as void* so this header stays
// Windows-free); the return value matches MessageBoxW.
int MessageBoxJp(void* owner, const char* body, const char* caption,
                 unsigned int flags);
}
