package org.libsdl.app;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.os.Build;
import android.os.Bundle;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;

public class SDLActivity extends Activity implements View.OnSystemUiVisibilityChangeListener {
    protected static SDLActivity instance;
    protected static SDLSurfaceView surfaceView;

    /** Standard SDL2 JNI functions (implemented in libSDL2.so) */
    public static native void onNativeTouch(int touchDevId, int pointerFingerId, int action, float x, float y, float p);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        instance = this;

        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN
        );
        getWindow().addFlags(
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
        );

        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);

        surfaceView = new SDLSurfaceView(this);
        setContentView(surfaceView);

        hideSystemUI();
        surfaceView.setOnSystemUiVisibilityChangeListener(this);
    }

    protected void hideSystemUI() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            surfaceView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LOW_PROFILE
            );
        }
    }

    @Override
    public void onSystemUiVisibilityChange(int visibility) {
        if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0) {
            hideSystemUI();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        instance = null;
    }

    public static void vibrate(int durationMs) {
        if (instance != null) {
            Vibrator vibrator = (Vibrator) instance.getSystemService(Context.VIBRATOR_SERVICE);
            if (vibrator != null && vibrator.hasVibrator()) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    vibrator.vibrate(VibrationEffect.createOneShot(durationMs, VibrationEffect.DEFAULT_AMPLITUDE));
                } else {
                    vibrator.vibrate(durationMs);
                }
            }
        }
    }

    static class SDLSurfaceView extends SurfaceView implements SurfaceHolder.Callback {
        public SDLSurfaceView(Context context) {
            super(context);
            getHolder().addCallback(this);
            setFocusable(true);
            setFocusableInTouchMode(true);
            requestFocus();
        }

        @Override
        public void surfaceCreated(SurfaceHolder holder) {}

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {}

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {}

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            int action = event.getActionMasked();
            int actionCode;
            switch (action) {
                case MotionEvent.ACTION_DOWN:
                case MotionEvent.ACTION_POINTER_DOWN:
                    actionCode = 0;
                    break;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_POINTER_UP:
                    actionCode = 1;
                    break;
                case MotionEvent.ACTION_MOVE:
                    actionCode = 2;
                    break;
                case MotionEvent.ACTION_CANCEL:
                    actionCode = 3;
                    break;
                default:
                    return true;
            }

            float w = (float) getWidth();
            float h = (float) getHeight();

            int pointerCount = event.getPointerCount();
            for (int i = 0; i < pointerCount; i++) {
                int id = event.getPointerId(i);
                float x = event.getX(i) / w;
                float y = event.getY(i) / h;
                float p = event.getPressure(i);

                onNativeTouch(0, id, actionCode, x, y, p);
            }

            return true;
        }

        @Override
        public boolean onKeyDown(int keyCode, KeyEvent event) {
            return super.onKeyDown(keyCode, event);
        }

        @Override
        public boolean onKeyUp(int keyCode, KeyEvent event) {
            return super.onKeyUp(keyCode, event);
        }
    }
}
