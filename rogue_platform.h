#ifndef ROGUE_PLATFORM_H
#define ROGUE_PLATFORM_H

#include <stddef.h>

void rogue_platform_configure_storage(const char *asset_root,
				      const char *user_root);
const char *rogue_platform_asset_path(const char *relative, char *out,
				      size_t out_size);
const char *rogue_platform_user_path(const char *relative, char *out,
				     size_t out_size);
char *rogue_platform_read_text_file(const char *relative_path);

#endif
