#include "rogue_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ROGUE_ANDROID
#include <jni.h>
#endif

#define ROGUE_PLATFORM_MAX_PATH 512
#define ROGUE_PLATFORM_MAX_TEXT 262144

static char asset_root[ROGUE_PLATFORM_MAX_PATH] = "";
static char user_root[ROGUE_PLATFORM_MAX_PATH] = "";
static int safe_area_left = 0;
static int safe_area_top = 0;
static int safe_area_right = 0;
static int safe_area_bottom = 0;

static void
copy_root(char *target, const char *source)
{
    if (source == NULL)
	source = "";
    strncpy(target, source, ROGUE_PLATFORM_MAX_PATH - 1);
    target[ROGUE_PLATFORM_MAX_PATH - 1] = '\0';
}

static const char *
join_platform_path(const char *root, const char *relative, char *out,
		   size_t out_size)
{
    size_t root_len;

    if (relative == NULL)
	relative = "";
    if (out == NULL || out_size == 0)
	return relative;

    if (root == NULL || root[0] == '\0')
    {
	snprintf(out, out_size, "%s", relative);
	return out;
    }

    root_len = strlen(root);
    if (root[root_len - 1] == '/' || root[root_len - 1] == '\\'
	|| relative[0] == '/' || relative[0] == '\\')
	snprintf(out, out_size, "%s%s", root, relative);
    else
	snprintf(out, out_size, "%s/%s", root, relative);
    return out;
}

void
rogue_platform_configure_storage(const char *new_asset_root,
				 const char *new_user_root)
{
    copy_root(asset_root, new_asset_root);
    copy_root(user_root, new_user_root);
}

const char *
rogue_platform_asset_path(const char *relative, char *out, size_t out_size)
{
    return join_platform_path(asset_root, relative, out, out_size);
}

const char *
rogue_platform_user_path(const char *relative, char *out, size_t out_size)
{
    return join_platform_path(user_root, relative, out, out_size);
}

char *
rogue_platform_read_text_file(const char *relative_path)
{
    char path[ROGUE_PLATFORM_MAX_PATH];
    FILE *file;
    long size;
    char *text;

    file = fopen(rogue_platform_asset_path(relative_path, path, sizeof(path)),
		 "rb");
    if (file == NULL)
	return NULL;

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    if (size < 0 || size > ROGUE_PLATFORM_MAX_TEXT)
    {
	fclose(file);
	return NULL;
    }

    fseek(file, 0, SEEK_SET);
    text = (char *) calloc((size_t) size + 1, 1);
    if (text == NULL)
    {
	fclose(file);
	return NULL;
    }

    fread(text, 1, (size_t) size, file);
    fclose(file);
    return text;
}

static int
clamp_nonnegative(int value)
{
    return value < 0 ? 0 : value;
}

void
rogue_platform_configure_safe_area(int left, int top, int right, int bottom)
{
    safe_area_left = clamp_nonnegative(left);
    safe_area_top = clamp_nonnegative(top);
    safe_area_right = clamp_nonnegative(right);
    safe_area_bottom = clamp_nonnegative(bottom);
}

int
rogue_platform_android_safe_top_inset(void)
{
    return safe_area_top;
}

#ifdef ROGUE_ANDROID
JNIEXPORT void JNICALL
Java_com_roguetiles_RogueTilesActivity_nativeConfigureStorage(
    JNIEnv *env, jclass clazz, jstring asset_root_jstring,
    jstring user_root_jstring)
{
    const char *asset_root_chars = NULL;
    const char *user_root_chars = NULL;

    (void) clazz;
    if (asset_root_jstring != NULL)
    {
	asset_root_chars = (*env)->GetStringUTFChars(env, asset_root_jstring,
						     NULL);
	if (asset_root_chars == NULL)
	    return;
    }

    if (user_root_jstring != NULL)
    {
	user_root_chars = (*env)->GetStringUTFChars(env, user_root_jstring,
						   NULL);
	if (user_root_chars == NULL)
	{
	    if (asset_root_chars != NULL)
		(*env)->ReleaseStringUTFChars(env, asset_root_jstring,
					      asset_root_chars);
	    return;
	}
    }

    rogue_platform_configure_storage(asset_root_chars, user_root_chars);

    if (asset_root_chars != NULL)
	(*env)->ReleaseStringUTFChars(env, asset_root_jstring,
				      asset_root_chars);
    if (user_root_chars != NULL)
	(*env)->ReleaseStringUTFChars(env, user_root_jstring, user_root_chars);
}

JNIEXPORT void JNICALL
Java_com_roguetiles_RogueTilesActivity_nativeConfigureSafeArea(
    JNIEnv *env, jclass clazz, jint left, jint top, jint right, jint bottom)
{
    (void) env;
    (void) clazz;
    rogue_platform_configure_safe_area((int) left, (int) top, (int) right,
				       (int) bottom);
}
#endif
