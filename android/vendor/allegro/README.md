# Allegro Android Dependency

Place the Android Allegro artifacts here before building the APK:

- `allegro-release.aar`
- `jni/arm64-v8a/liballegro.so`
- `jni/arm64-v8a/liballegro_image.so`
- `jni/arm64-v8a/liballegro_font.so`
- `jni/arm64-v8a/liballegro_ttf.so`
- `jni/arm64-v8a/liballegro_primitives.so`
- `jni/x86_64/liballegro.so`
- `jni/x86_64/liballegro_image.so`
- `jni/x86_64/liballegro_font.so`
- `jni/x86_64/liballegro_ttf.so`
- `jni/x86_64/liballegro_primitives.so`

The app loads these libraries from `RogueTilesActivity` before Allegro starts `libroguetiles.so`.
