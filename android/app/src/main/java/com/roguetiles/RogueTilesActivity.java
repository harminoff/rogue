package com.roguetiles;

import android.os.Bundle;
import org.liballeg.android.AllegroActivity;

public class RogueTilesActivity extends AllegroActivity {
    static {
        System.loadLibrary("allegro");
        System.loadLibrary("allegro_image");
        System.loadLibrary("allegro_font");
        System.loadLibrary("allegro_ttf");
        System.loadLibrary("allegro_primitives");
        System.loadLibrary("roguetiles");
    }

    public RogueTilesActivity() {
        super("libroguetiles.so");
    }

    private static native void nativeConfigureStorage(String assetRoot, String userRoot);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        String assetRoot = AndroidAssetSync.syncBundledAssets(this).getAbsolutePath();
        String userRoot = getFilesDir().getAbsolutePath();
        nativeConfigureStorage(assetRoot, userRoot);
        super.onCreate(savedInstanceState);
    }
}
