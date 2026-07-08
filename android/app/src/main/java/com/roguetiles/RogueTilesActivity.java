package com.roguetiles;

import android.os.Build;
import android.os.Bundle;
import android.view.DisplayCutout;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowManager;
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
    private static native void nativeConfigureSafeArea(int left, int top, int right, int bottom);

    @Override
    public void onCreate(Bundle savedInstanceState) {
        String assetRoot = AndroidAssetSync.syncBundledAssets(this).getAbsolutePath();
        String userRoot = getFilesDir().getAbsolutePath();
        nativeConfigureStorage(assetRoot, userRoot);
        nativeConfigureSafeArea(0, 0, 0, 0);
        super.onCreate(savedInstanceState);
        installSafeAreaListener();
    }

    private void installSafeAreaListener() {
        Window window = getWindow();
        if (window == null) {
            return;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            WindowManager.LayoutParams params = window.getAttributes();
            params.layoutInDisplayCutoutMode =
                    WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            window.setAttributes(params);
        }

        View decorView = window.getDecorView();
        if (decorView == null) {
            return;
        }

        decorView.setOnApplyWindowInsetsListener((view, insets) -> {
            configureSafeAreaFromInsets(insets);
            return insets;
        });
        decorView.requestApplyInsets();
        configureSafeAreaFromInsets(decorView.getRootWindowInsets());
    }

    private static void configureSafeAreaFromInsets(WindowInsets insets) {
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;

        if (insets != null) {
            left = Math.max(left, insets.getSystemWindowInsetLeft());
            top = Math.max(top, insets.getSystemWindowInsetTop());
            right = Math.max(right, insets.getSystemWindowInsetRight());
            bottom = Math.max(bottom, insets.getSystemWindowInsetBottom());

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                DisplayCutout cutout = insets.getDisplayCutout();
                if (cutout != null) {
                    left = Math.max(left, cutout.getSafeInsetLeft());
                    top = Math.max(top, cutout.getSafeInsetTop());
                    right = Math.max(right, cutout.getSafeInsetRight());
                    bottom = Math.max(bottom, cutout.getSafeInsetBottom());
                }
            }
        }

        nativeConfigureSafeArea(left, top, right, bottom);
    }
}
