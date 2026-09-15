#include "../ThirdParty/brotli/c/common/dictionary.h"

// The build uses a Brotli encoder with static-dictionary matching disabled. Keeping an empty
// definition here avoids embedding the otherwise unused 122784-byte word list.
static const BrotliDictionary g_EmptyDictionary = {{0}, {0}, 0, NULL};

const BrotliDictionary* BrotliGetDictionary(void)
{
	return &g_EmptyDictionary;
}

void BrotliSetDictionaryData(const uint8_t* data)
{
	BROTLI_UNUSED(data);
}
