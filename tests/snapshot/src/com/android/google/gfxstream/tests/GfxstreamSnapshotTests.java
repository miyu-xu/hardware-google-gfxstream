/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.google.gfxstream.tests;

import static org.junit.Assert.assertTrue;

import com.android.tradefed.config.Option;
import com.android.tradefed.device.internal.DeviceResetHandler;
import com.android.tradefed.device.internal.DeviceSnapshotHandler;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.awt.Color;
import java.awt.image.BufferedImage;
import java.io.File;
import java.util.UUID;

import javax.imageio.ImageIO;

/**
 * Test snapshot/restore function.
 *
 * <p>* This test resets the device thus it should not run with other tests in the same test suite
 * to avoid unexpected behavior.
 *
 * <p>* The test logic relies on cvd and snapshot_util_cvd tools, so it can only run in a test lab
 * setup.
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class GfxstreamSnapshotTests extends BaseHostJUnit4Test {

    private static final long DEFAULT_TIMEOUT_MS = 5000;

    private static final int SCREENSHOT_CHECK_ATTEMPTS = 5;

    private static final int SCREENSHOT_CHECK_TIMEOUT_MILLISECONDS = 1000;

    private static final String VK_SAMPLES_APP_APK = "GfxstreamVulkanSamplesApp.apk";

    private static final String VK_SAMPLES_PKG = "com.android.google.gfxstream.samples";

    private static final String VK_SAMPLES_FULLSCREEN_COLOR_ACTIVITY =
        "com.android.google.gfxstream.samples/.FullscreenColorActivity";

    @Before
    public void setUp() throws Exception {
        getDevice().uninstallPackage(VK_SAMPLES_APP_APK);
        installPackage(VK_SAMPLES_APP_APK);
    }

    @After
    public void tearDown() throws Exception {
        getDevice().uninstallPackage(VK_SAMPLES_APP_APK);
    }

    private BufferedImage getScreenshot() throws Exception {
        InputStreamSource screenshotStream = getDevice().getScreenshot();
        if (screenshotStream == null) {
            Assert.fail("Failed to get screenshot: stream is null?");
        }
        return ImageIO.read(screenshotStream.createInputStream());
    }

    private static final int PIXEL_DIFFERENCE_THRESHOLD = 16;

    private boolean isAproximatelyEqual(Color actual, Color expected) {
        int diff = Math.abs(actual.getRed() - expected.getRed()) +
                   Math.abs(actual.getGreen() - expected.getGreen()) +
                   Math.abs(actual.getBlue() - expected.getBlue());
        return diff <= PIXEL_DIFFERENCE_THRESHOLD;
    }

    private void waitForColor(Color expected) throws Exception {
        for (int attempt = 0; attempt < SCREENSHOT_CHECK_ATTEMPTS; attempt++) {
            BufferedImage screenshot = getScreenshot();

            final Color actual = new Color(screenshot.getRGB(screenshot.getWidth() / 2, screenshot.getHeight() / 2));
            if (isAproximatelyEqual(actual, expected)) {
                return;
            }

            Thread.sleep(SCREENSHOT_CHECK_TIMEOUT_MILLISECONDS);
        }
        throw new IllegalStateException("Failed to find color on display.");
    }

    @Test
    public void testFullscreenColorSnapshot() throws Exception {
        final String snapshotId = "snapshot_" + UUID.randomUUID().toString();

        // Reboot to make sure device isn't dirty from previous tests.
        getDevice().reboot();

        getDevice().executeShellCommand(String.format("am start -n %s", VK_SAMPLES_FULLSCREEN_COLOR_ACTIVITY));
        waitForColor(Color.RED);

        // Snapshot the device
        new DeviceSnapshotHandler().snapshotDevice(getDevice(), snapshotId);

        try {
            new DeviceSnapshotHandler().restoreSnapshotDevice(getDevice(), snapshotId);
            waitForColor(Color.RED);
        } finally {
            new DeviceSnapshotHandler().deleteSnapshot(getDevice(), snapshotId);
        }
    }
}