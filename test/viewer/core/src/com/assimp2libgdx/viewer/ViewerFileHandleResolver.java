package com.assimp2libgdx.viewer;

import com.badlogic.gdx.Gdx;
import com.badlogic.gdx.assets.loaders.FileHandleResolver;
import com.badlogic.gdx.files.FileHandle;

public class ViewerFileHandleResolver implements FileHandleResolver
{
	private static FileHandle root = Gdx.files.local("core/outputs");
	
	public FileHandle resolve (String fileName) {
		return root.child(fileName);
	}
}
