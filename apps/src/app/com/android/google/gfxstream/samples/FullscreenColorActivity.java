// Copyright (C) 2024 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.android.google.gfxstream.samples;

import android.app.NativeActivity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

/**
 * A simple activity to render a color to the entire screen.
 */
public class FullscreenColorActivity extends NativeActivity {

    private static final String TAG = FullscreenColorActivity.class.getSimpleName();

    static {
        System.loadLibrary("gfxstream_vulkan_samples_jni");
    }

    protected void onResume() {
        Log.i(TAG, "Activity onStart().");
        super.onResume();
    }

    @Override
    public void onCreate(Bundle bundle) {
        Log.i(TAG, "Activity onCreate().");
        super.onCreate(bundle);
    }

    @Override
    public void onStart() {
        Log.i(TAG, "Activity onStart().");
        super.onStart();
    }
}