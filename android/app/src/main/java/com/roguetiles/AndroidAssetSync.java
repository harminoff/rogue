package com.roguetiles;

import android.content.Context;
import android.content.res.AssetManager;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

final class AndroidAssetSync {
    private AndroidAssetSync() {
    }

    static File syncBundledAssets(Context context) {
        File root = new File(context.getFilesDir(), "bundled-assets");
        deleteTree(root);
        copyTree(context.getAssets(), "assets", root);
        copyTree(context.getAssets(), "tilepacks", root);
        copyTree(context.getAssets(), "variants", root);
        copyTree(context.getAssets(), "rogue54.6", root);
        copyTree(context.getAssets(), "rogue54.doc", root);
        return root;
    }

    private static void deleteTree(File target) {
        if (!target.exists()) {
            return;
        }
        if (target.isDirectory()) {
            File[] children = target.listFiles();
            if (children == null) {
                throw new IllegalStateException("Could not list bundled asset directory: " + target);
            }
            for (File child : children) {
                deleteTree(child);
            }
        }
        if (!target.delete()) {
            throw new IllegalStateException("Could not delete bundled asset path: " + target);
        }
    }

    private static void copyTree(AssetManager assets, String source, File root) {
        try {
            String[] children = assets.list(source);
            if (children == null || children.length == 0) {
                copyFile(assets, source, new File(root, source));
                return;
            }
            for (String child : children) {
                copyTree(assets, source + "/" + child, root);
            }
        } catch (IOException ex) {
            throw new IllegalStateException("Could not copy bundled asset: " + source, ex);
        }
    }

    private static void copyFile(AssetManager assets, String source, File destination) throws IOException {
        File parent = destination.getParentFile();
        if (parent != null && !parent.exists() && !parent.mkdirs()) {
            throw new IOException("Could not create directory: " + parent);
        }
        try (InputStream input = assets.open(source);
             OutputStream output = new FileOutputStream(destination)) {
            byte[] buffer = new byte[8192];
            int read;
            while ((read = input.read(buffer)) >= 0) {
                output.write(buffer, 0, read);
            }
        }
    }
}
