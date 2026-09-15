#pragma once

#include <stdint.h>

// The archive is intentionally simpler than a general container format: entries are consumed in
// order after Brotli decompression and need only a destination path and platform selector.
static constexpr uint32_t g_ArchiveMagic = 0x524D4252;
static constexpr uint32_t g_ArchiveVersion = 1;
static constexpr uint32_t g_ArchiveFooterMagic = 0x524D454E;

enum ArchiveArchitecture : uint8_t
{
	ArchiveArchitectureAny,
	ArchiveArchitecture32,
	ArchiveArchitecture64
};

#pragma pack(push, 1)
struct ArchiveHeader
{
	uint32_t magic;
	uint32_t version;
	uint32_t fileCount;
};

struct ArchiveEntry
{
	uint16_t pathLength;
	uint8_t architecture;
	uint8_t flags;
	uint64_t size;
};

struct ArchiveFooter
{
	uint32_t magic;
	uint32_t version;
	uint64_t archiveOffset;
	uint64_t compressedSize;
	uint64_t uncompressedSize;
};
#pragma pack(pop)

static constexpr uint8_t g_ArchiveFlagStandardOnly = 1;
