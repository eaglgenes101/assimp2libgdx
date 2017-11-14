package com.assimp2libgdx.viewer.desktop;

import com.assimp2libgdx.viewer.ModelViewer;
import com.badlogic.gdx.backends.lwjgl.LwjglApplication;
import com.badlogic.gdx.backends.lwjgl.LwjglApplicationConfiguration;

public class DesktopLauncher
{
	public static void main (String[] arg) {
		LwjglApplicationConfiguration config = new LwjglApplicationConfiguration();
		config.title = "Model-Viewer";
		config.width = 640;
		config.height = 480;
		config.foregroundFPS = 60;
		config.backgroundFPS = 60;
		new LwjglApplication(new ModelViewer(arg[0]), config);
	}
}
