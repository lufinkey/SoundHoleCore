package com.lufinkey.soundholecoretest;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import com.lufinkey.soundholecore.SoundHole;

public class MainActivity extends AppCompatActivity {
	static {
		System.loadLibrary("TestApp");
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		System.out.println("initializing SoundHole");
		SoundHole.init(getApplicationContext());
		runTests();
	}

	/**
	 * A native method that is implemented by the 'native-lib' native library,
	 * which is packaged with this application.
	 */
	public native void runTests();
}
