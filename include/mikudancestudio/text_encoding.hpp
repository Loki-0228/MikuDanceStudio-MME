#pragma once

#include <cstddef>
#include <string>

namespace mikudancestudio::mdl { struct ModelRecord; }
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
}
