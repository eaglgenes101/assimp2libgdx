/*
Assimp2Libgdx
Copyright (c) 2017, Eugene Wang

Licensed under a 3-clause BSD license. See the LICENSE file for more information.
Based on Assimp2Libgdx, whose license is below: 
*/
/*
Assimp2Libgdx
Copyright (c) 2011, Alexander C. Gessler

Licensed under a 3-clause BSD license. See the LICENSE file for more information.
*/
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>

#include <assimp/defs.h>
#include <assimp/scene.h>

#include <sstream>
#include <iostream>
#include <limits>
#include <cassert>
#include <cstring>
#include <set>

#define CURRENT_FORMAT_VERSION 03

#include <memory>

#include "mesh_splitter.h"

namespace {
void Assimp2Libgdx(const char*, Assimp::IOSystem*, const aiScene*, const Assimp::ExportProperties*);
}

Assimp::Exporter::ExportFormatEntry Assimp2Libgdx_desc = Assimp::Exporter::ExportFormatEntry(
	"g3dj",
	"LibGDX 3D Format (json)",
	"g3dj",
	Assimp2Libgdx,
	0u);

namespace {


	// small utility class to simplify serializing the aiScene to Json
class JSONWriter
{

public:

	enum {
		Flag_DoNotIndent = 0x1,
		Flag_WriteSpecialFloats = 0x2,
	};

public:

	JSONWriter(Assimp::IOStream& out, unsigned int flags = 0u) : out(out), flags(flags)
	{
		// make sure that all formatting happens using the standard, C locale and not the user's current locale
		buff.imbue( std::locale("C") );
		first = true;
		doDelimit = true;
	}

	~JSONWriter()
	{
		Flush();
	}

public:

	void Flush()	{
		const std::string s = buff.str();
		out.Write(s.c_str(),s.length(),1);
		buff.clear();
	}

	void PushIndent() {
		indent += '\t';
	}

	void PopIndent() {
		indent.erase(indent.end()-1);
	}

	void Key(const std::string& name) {
		Delimit();
		NewLine();
		AddIndentation();
		doDelimit = false;
		buff << '\"'+name+"\": ";
	}

	template<typename Literal>
	void SimpleValue(const Literal& s) {
		if (doDelimit) {
			Delimit();
			NewLine();
			AddIndentation();
		}
		doDelimit = true;
		LiteralToString(buff, s);
	}

	void StartObj() {
		if (doDelimit) {
			Delimit();
			NewLine();
			AddIndentation();
		}
		doDelimit = true;
		first = true;
		buff << "{";
		PushIndent();
	}

	void EndObj() {
		PopIndent();
		NewLine();
		AddIndentation();
		doDelimit = true;
		first = false;
		buff << "}";
	}

	void StartArray() {
		if (doDelimit) {
			Delimit();
			NewLine();
			AddIndentation();
		}
		doDelimit = true;
		first = true;
		buff << "[";
		PushIndent();
	}

	void EndArray() {
		PopIndent();
		NewLine();
		AddIndentation();
		buff << "]";
		doDelimit = true;
		first = false;
	}

	void AddIndentation() {
		if(!(flags & Flag_DoNotIndent)) {
			buff << indent;
		}
	}
	
	void NewLine() {
		buff << std::endl;
	}

	void Delimit() {
		if(!first) {
			buff << ',';
		}
		else {
			buff << ' ';		
			first = false;
		}
	}

private:

	//To prevent errors, the generic version is not enabled
	//Use one of the specializations instead
	/*
	template<typename Literal>
	std::stringstream& LiteralToString(std::stringstream& stream, const Literal& s) {
		stream << s;
		return stream;
	}
	*/
	
	std::stringstream& LiteralToString(std::stringstream& stream, const int& s) {
		stream << s;
		return stream;
	}
	
	std::stringstream& LiteralToString(std::stringstream& stream, const unsigned int& s) {
		stream << s;
		return stream;
	}
	
	std::stringstream& LiteralToString(std::stringstream& stream, const std::string& s) {
		std::string t;
		// escape backslashes and single quotes, both would render the JSON invalid if left as is
		//t.reserve(s.size()); //BZZZZZT
		for(unsigned int i = 0; i < s.size(); i++) {
			
			if (s[i] == '\\' || s[i] == '\'' || s[i] == '\"') {
				t.push_back('\\');
			}

			t.push_back(s[i]);
		}
		stream << "\"";
		stream << t;
		stream << "\"";
		return stream;
	}

	std::stringstream& LiteralToString(std::stringstream& stream, const char* s) {
		std::string t;

		// escape backslashes and single quotes, both would render the JSON invalid if left as is
		//t.reserve(strlen(s)); //BZZZZZT
		for(const char* i = s; *i != '\0'; i++) {
			
			if (*i == '\\' || *i == '\'' || *i == '\"') {
				t.push_back('\\');
			}

			t.push_back(*i);
		}
		stream << "\"";
		stream << t;
		stream << "\"";
		return stream;
	}

	std::stringstream& LiteralToString(std::stringstream& stream, const float& f) {
		if (!std::numeric_limits<float>::is_iec559) {
			// on a non IEEE-754 platform, we make no assumptions about the representation or existence
			// of special floating-point numbers. 
			stream << f;
			return stream;
		}
		// JSON does not support writing Inf/Nan
		// [RFC 4672: "Numeric values that cannot be represented as sequences of digits
		// (such as Infinity and NaN) are not permitted."]
		// Nevertheless, many parsers will accept the special keywords Infinity, -Infinity and NaN
		if (std::numeric_limits<float>::infinity() == fabs(f)) {
			if (flags & Flag_WriteSpecialFloats) {
				stream << (f < 0 ? "\"-" : "\"") + std::string( "Infinity\"" );
				return stream;
			}
		//  we should print this warning, but we can't - this is called from within a generic assimp exporter, we cannot use cerr
		//	std::cerr << "warning: cannot represent infinite number literal, substituting 0 instead (use -i flag to enforce Infinity/NaN)" << std::endl;
			stream << "0.0";
			return stream;
		}
		// f!=f is the most reliable test for NaNs that I know of
		else if (f != f) {
			if (flags & Flag_WriteSpecialFloats) {
				stream << "\"NaN\"";
				return stream;
			}
		//  we should print this warning, but we can't - this is called from within a generic assimp exporter, we cannot use cerr
		//	std::cerr << "warning: cannot represent infinite number literal, substituting 0 instead (use -i flag to enforce Infinity/NaN)" << std::endl;
			stream << "0.0";
			return stream;
		}
		stream << f;
		return stream;
	}

private: 
	Assimp::IOStream& out;
	std::string indent, newline;
	std::stringstream buff;
	bool first;
	bool doDelimit;

	unsigned int flags;
};

///////////////////////////////////////////////////////////
// Modified below
///////////////////////////////////////////////////////////

void Write(JSONWriter& out, const aiVector3D& ai) 
{
	out.SimpleValue(ai.x);
	out.SimpleValue(ai.y);
	out.SimpleValue(ai.z);
}

void Write(JSONWriter& out, const aiQuaternion& ai) 
{
	out.SimpleValue(ai.w);
	out.SimpleValue(ai.x);
	out.SimpleValue(ai.y);
	out.SimpleValue(ai.z);
}

void Write(JSONWriter& out, const aiColor4D& ai) 
{
	out.SimpleValue(ai.r);
	out.SimpleValue(ai.g);
	out.SimpleValue(ai.b);
	out.SimpleValue(ai.a);
}

void Write(JSONWriter& out, const aiBone& ai)
{
	out.StartObj();
	
	//...I think
	//Or is it the node ID that should be here? 
	out.Key("node");
	out.SimpleValue(ai.mName.C_Str()); 
	
	aiMatrix4x4 transform = ai.mOffsetMatrix;
	//Decompose
	aiVector3t<float_t> scale;
	aiQuaterniont<float_t> rot;
	aiVector3t<float_t> disp;
	transform.Decompose(scale, rot, disp);
	
	out.Key("translation");
	out.StartArray();
	Write(out, disp);
	out.EndArray();
	
	out.Key("rotation");
	out.StartArray();
	Write(out, rot);
	out.EndArray();
	
	out.Key("scale");
	out.StartArray();
	Write(out, scale);
	out.EndArray();
	
	out.EndObj();
}


void Write(JSONWriter& out, const aiFace& ai)
{
	out.StartArray();
	for(unsigned int i = 0; i < ai.mNumIndices; ++i) {
		out.SimpleValue(ai.mIndices[i]);
	}
	out.EndArray();
}

template <typename Literal>
void WriteAttribute(JSONWriter& out, const Literal& usage, int size, std::string type = "FLOAT")
{
	out.StartObj();
	out.Key("usage");
	out.SimpleValue(usage);
	out.Key("size");
	out.SimpleValue(size);
	out.Key("type");
	out.SimpleValue(type);
	out.EndObj();
}

//For meshes
void Write(JSONWriter& out, const aiMesh& ai)
{
	out.StartObj(); 
	
	bool writePositions = false;
	bool writeNormals = false;
	bool writeColors = false;
	bool writeTangents = false; 
	out.Key("attributes");
	out.StartArray();
	if (ai.HasPositions()) {
		writePositions = true;
		WriteAttribute(out, "POSITION", 3);
	}
	if (ai.HasNormals()) {
		writeNormals = true;
		WriteAttribute(out, "NORMAL", 3);
	}
	if (ai.GetNumColorChannels()) {
		writeColors = true;
		WriteAttribute(out, "COLOR", ai.GetNumColorChannels()*4);
	}
	if (ai.HasTangentsAndBitangents()) {
		writeTangents = true;
		WriteAttribute(out, "TANGENT", 3);
		WriteAttribute(out, "BINORMAL", 3);
	}
	out.EndArray();
	
	out.Key("vertices");
	out.StartArray();
	for (unsigned int i = 0; i < ai.mNumVertices; ++i) {
		if (writePositions) Write(out, ai.mVertices[i]);
		if (writeNormals) Write(out, ai.mNormals[i]);
		if (ai.GetNumColorChannels()) {
			for (unsigned int j = 0; j < ai.GetNumColorChannels(); ++j) {
				if (writeColors) Write(out, ai.mColors[j][i]); //TODO: fix this up
			}
		}
		if (writeTangents) {
			Write(out, ai.mTangents[i]);
			Write(out, ai.mBitangents[i]);
		}
	}
	out.EndArray();
	
	out.Key("parts");
	out.StartArray();
	for (unsigned int i = 0; i < ai.mNumFaces; ++i) {
		out.StartObj();
		out.Key("id");
		//Name takes the form <meshName> "." <faceNum>
		out.SimpleValue(std::string(ai.mName.C_Str())+std::string(".")+std::to_string(i)); 
		out.Key("type");
		switch(ai.mFaces[i].mNumIndices)
		{
			case 1: //Point
				out.SimpleValue("POINTS");
				break;
			case 2: //Line
				out.SimpleValue("LINES");
				break;
			case 3: 
				out.SimpleValue("TRIANGLES");
				break;
			default:
				out.SimpleValue("TRIANGLE_STRIP");
				break;
		}
		out.Key("indices");
		Write(out, ai.mFaces[i]);
		out.EndObj();
	}
	out.EndArray();

	out.EndObj();
}

void WriteAsPart(JSONWriter& out, const aiMesh& ai, int id)
{
	out.StartObj();
	out.Key("meshpartid");
	//Name takes the form <meshName> "." <faceNum>
	out.SimpleValue(std::string(ai.mName.C_Str())+std::string(".")+std::to_string(id));
	out.Key("materialid");
	out.SimpleValue(ai.mMaterialIndex);
	if (ai.HasBones()) {
		assert(ai.mNumBones > 0);
		assert(ai.mBones != nullptr);
		//assert(*ai.mBones != nullptr); 
		
		//TODO: Figure out how to access bones without segfaulting
		out.Key("bones");
		out.StartArray();
		for (unsigned int i = 0; i < ai.mNumBones; ++i) {
			Write(out, *ai.mBones[i] ); //Why does this segfault
		}
		out.EndArray();
	}
	if (ai.GetNumUVChannels()) {
		//TODO: Figure out how to access UV maps without segfaulting
		
		out.Key("uvMapping");
		out.StartArray();
		for (unsigned int channel = 0; channel < ai.GetNumUVChannels(); ++channel) {
			out.StartArray();
			unsigned int components = ai.mNumUVComponents[channel] ? ai.mNumUVComponents[channel] : 2;
			for (unsigned int vertex = 0; vertex < ai.mNumVertices; ++vertex) {
				//The spec isn't entirely clear, so this is my best guess
				//It spills the vector coordinates into one homogenous array
				for (unsigned int c = 0; c < components; ++c) {
					out.SimpleValue(ai.mTextureCoords[channel][vertex][c]); 
				}
			}
			out.EndArray();
		}
		out.EndArray();
		
	}
	out.EndObj();
}

//Recursive function, so we iterate through all nodes
void Write(JSONWriter& out, const aiNode& ai, aiMesh* meshes, int numMeshes)
{
	out.StartObj();

	out.Key("name");
	out.SimpleValue(ai.mName.C_Str());
	
	aiMatrix4x4 transform = ai.mTransformation;
	//Decompose
	aiVector3t<float_t> scale;
	aiQuaterniont<float_t> rot;
	aiVector3t<float_t> disp;
	transform.Decompose(scale, rot, disp);
	
	out.Key("translation");
	out.StartArray();
	Write(out, disp);
	out.EndArray();
	
	out.Key("rotation");
	out.StartArray();
	Write(out, rot);
	out.EndArray();
	
	out.Key("scale");
	out.StartArray();
	Write(out, scale);
	out.EndArray();

	if(ai.mNumMeshes) {
		out.Key("parts");
		out.StartArray();
		for(unsigned int n = 0; n < ai.mNumMeshes; ++n) {
			assert(ai.mMeshes[n] < numMeshes);
			WriteAsPart(out, meshes[ai.mMeshes[n]], n); //?
		}
		out.EndArray();
	}
	out.EndObj();

	//As said, recursion
	if(ai.mNumChildren) {
		for(unsigned int n = 0; n < ai.mNumChildren; ++n) {
			Write(out,*ai.mChildren[n],meshes,numMeshes);
		}
	}
}

std::array<aiString*, 4> Write(JSONWriter& out, const aiMaterial& ai, int d)
{
	aiString* val[4];
	out.Key("id");
	out.SimpleValue(std::to_string(d));
	//Stuff to defer until later
	bool doBlend = false;
	float opacity = 1.0;
	const char* srcBlend = nullptr;
	const char* destBlend = nullptr;
	int diffuseDepth = -1;
	aiString* diffusePath = nullptr;
	int specularDepth = -1;
	aiString* specularPath = nullptr;
	int bumpDepth = -1;
	aiString* bumpPath = nullptr;
	int normalDepth = -1;
	aiString* normalPath = nullptr;
	for (unsigned int i = 0; i < ai.mNumProperties; i++) {
		const aiMaterialProperty* prop = ai.mProperties[i];
		//Took me forever to figure out what was going on before finding that 
		//the macros weren't just string literals
		//Don't do unhygenic macros, kids
		if (!strcmp(prop->mKey.C_Str(), "$clr.diffuse")) {
			out.Key("diffuseColor");
			out.StartArray();
			for(unsigned int i = 0; i < prop->mDataLength/sizeof(float); ++i) {
				out.SimpleValue(reinterpret_cast<float*>(prop->mData)[i]);
			}
			out.EndArray();
			break;
		}
		else if (!strcmp(prop->mKey.C_Str(), "$clr.specular")) {
			out.Key("specularColor");
			out.StartArray();
			for(unsigned int i = 0; i < prop->mDataLength/sizeof(float); ++i) {
				out.SimpleValue(reinterpret_cast<float*>(prop->mData)[i]);
			}
			out.EndArray();
			break;
		}
		else if (!strcmp(prop->mKey.C_Str(), "$clr.ambient")) {
			out.Key("ambientColor");
			out.StartArray();
			for(unsigned int i = 0; i < prop->mDataLength/sizeof(float); ++i) {
				out.SimpleValue(reinterpret_cast<float*>(prop->mData)[i]);
			}
			out.EndArray();
			break;
		}
		else if (!strcmp(prop->mKey.C_Str(), "$clr.emissive")) {
			out.Key("emissiveColor");
			out.StartArray();
			for(unsigned int i = 0; i < prop->mDataLength/sizeof(float); ++i) {
				out.SimpleValue(reinterpret_cast<float*>(prop->mData)[i]);
			}
			out.EndArray();
			break;
		}
		else if (!strcmp(prop->mKey.C_Str(), "$mat.twosided")) {
			out.Key("cullface");
			bool keepBack = *reinterpret_cast<bool*>(prop->mData);
			if (keepBack) out.SimpleValue("NONE");
			else out.SimpleValue("BACK");
			break;
		}
		else if (!strcmp(prop->mKey.C_Str(), "$mat.blend")) {
			//Modify blend
			doBlend = true;
			if (*reinterpret_cast<unsigned int*>(prop->mData) == aiBlendMode_Additive)
			{
				srcBlend = "ONE";
				destBlend = "ONE";
			}
			else
			{
				srcBlend = "SRC_ALPHA";
				doBlend = "ONE_MINUS_SRC_ALPHA";
			}	
			break;
		}
		else if (!strcmp(prop->mKey.C_Str(), "$mat.opacity")) {
			doBlend = true;
			opacity = *reinterpret_cast<float*>(prop->mData);
			break;
		}
		else if (!strcmp(prop->mKey.C_Str(), "$mat.shininess")) {
			out.Key("shininess");
			out.SimpleValue(*reinterpret_cast<float*>(prop->mData));
			break;
		}
		else if (!strcmp(prop->mKey.C_Str(), _AI_MATKEY_TEXTURE_BASE)) {
			long layer = prop->mIndex;
			unsigned int type = prop->mSemantic;
			switch (type)
			{
				case aiTextureType_DIFFUSE:
				{
					if (layer > diffuseDepth)
					{
						diffuseDepth = layer;
						diffusePath = reinterpret_cast<aiString*>(prop->mData);
					}
					break;
				}
				case aiTextureType_SPECULAR: 
				{
					if (layer > specularDepth)
					{
						specularDepth = layer;
						specularPath = reinterpret_cast<aiString*>(prop->mData);
					}
					break;
				}
				case aiTextureType_HEIGHT:
				case aiTextureType_DISPLACEMENT:
				{
					if (layer > bumpDepth)
					{
						bumpDepth = layer;
						bumpPath = reinterpret_cast<aiString*>(prop->mData);
					}
					break;
				}
				case aiTextureType_NORMALS:
				{
					if (layer > normalDepth)
					{
						normalDepth = layer;
						normalPath = reinterpret_cast<aiString*>(prop->mData);
					}
				}
			}
		}
	}
	if (doBlend)
	{
		out.Key("blended");
		out.StartObj();
		out.Key("opacity");
		out.SimpleValue(opacity);
		if (srcBlend != nullptr) 
		{
			out.Key("source");
			out.SimpleValue(srcBlend);
		}
		if (destBlend != nullptr)
		{
			out.Key("destination");
			out.SimpleValue(destBlend);
		}
		out.EndObj();
	}
	if (diffuseDepth >= 0)
	{
		out.Key("diffuseTexture");
		assert(diffusePath != nullptr);
		out.SimpleValue(diffusePath->C_Str());
		val[0] = diffusePath;
	}
	else
		val[0] = nullptr;
	if (specularDepth >= 0)
	{
		out.Key("specularTexture");
		assert(specularPath != nullptr);
		out.SimpleValue(specularPath->C_Str());
		val[1] = specularPath;
	}
	else
		val[1] = nullptr;
	if (bumpDepth >= 0)
	{
		out.Key("bumpTexture");
		assert(bumpPath != nullptr);
		out.SimpleValue(bumpPath->C_Str());
		val[2] = bumpPath;
	}
	else
		val[2] = nullptr;
	if (normalDepth >= 0)
	{
		out.Key("normalTexture");
		assert(normalPath != nullptr);
		out.SimpleValue(normalPath->C_Str());
		val[3] = normalPath;
	}
	else
		val[3] = nullptr;
	return {val[0], val[1], val[2], val[3]};
}

void Write(JSONWriter& out, const aiNodeAnim& ai)
{
	out.StartObj();
	
	out.Key("node");
	out.SimpleValue(ai.mNodeName.C_Str());
	
	std::map<float, aiVector3D> posKeys;
	std::map<float, aiQuaternion> rotKeys;
	std::map<float, aiVector3D> scaleKeys;
	
	//Obtain relevant keys
	for (unsigned int i = 0; i < ai.mNumPositionKeys; ++i) {
		posKeys[ai.mPositionKeys[i].mTime] = ai.mPositionKeys[i].mValue;
	}
	for (unsigned int i = 0; i < ai.mNumRotationKeys; ++i) {
		rotKeys[ai.mRotationKeys[i].mTime] = ai.mRotationKeys[i].mValue;
	}
	for (unsigned int i = 0; i < ai.mNumScalingKeys; ++i) {
		scaleKeys[ai.mScalingKeys[i].mTime] = ai.mScalingKeys[i].mValue;
	}
	
	out.Key("keyframes");
	out.StartArray();
	//Iterate over the union of float keyframes
	for (std::map<float, aiVector3D>::iterator i = posKeys.begin(); i != posKeys.end(); i++)
	{
		out.StartObj();
		out.Key("keytime");
		out.SimpleValue(i->first);
		out.Key("translation");
		out.StartArray();
		Write(out, i->second);
		out.EndArray();
		if (rotKeys.find(i->first) != rotKeys.end()) {
			out.Key("rotation");
			out.StartArray();
			Write(out, rotKeys[i->first]);
			out.EndArray();
		}
		if (scaleKeys.find(i->first) != scaleKeys.end()) {
			out.Key("scaling");
			out.StartArray();
			Write(out, rotKeys[i->first]);
			out.EndArray();
		}
		out.EndObj();
	}
	for (std::map<float, aiQuaternion>::iterator i = rotKeys.begin(); i != rotKeys.end(); i++)
	{
		if (posKeys.find(i->first) == posKeys.end()) {
			out.StartObj();
			out.Key("keytime");
			out.SimpleValue(i->first);
			out.Key("rotation");
			out.StartArray();
			Write(out, i->second);
			out.EndArray();
			if (scaleKeys.find(i->first) != scaleKeys.end()) {
				out.Key("scaling");
				out.StartArray();
				Write(out, rotKeys[i->first]);
				out.EndArray();
			}
			out.EndObj();
		}
	}
	for (std::map<float, aiVector3D>::iterator i = scaleKeys.begin(); i != scaleKeys.end(); i++)
	{
		if (posKeys.find(i->first) == posKeys.end() && rotKeys.find(i->first) == rotKeys.end()) {
			out.StartObj();
			out.Key("keytime");
			out.SimpleValue(i->first);
			out.Key("rotation");
			out.StartArray();
			Write(out, i->second);
			out.EndArray();
			out.EndObj();
		}
	}
	out.EndArray();
}

void Write(JSONWriter& out, const aiAnimation& ai)
{
	out.StartObj();

	out.Key("id");
	out.SimpleValue(ai.mName.C_Str());
	
	//TODO: Figure out what to do for mesh animations
	
	if (ai.mNumChannels > 0)
	{
		out.Key("nodes");
		out.StartArray();
		for(unsigned int n = 0; n < ai.mNumChannels; ++n) {
			Write(out,*ai.mChannels[n]);
		}
		out.EndArray();
	}
	out.EndObj();
}

void WriteVersionInfo(JSONWriter& out)
{
	out.StartArray();
	out.SimpleValue("0");
	out.SimpleValue("3"); //Specification this converter is based on
	out.EndArray();
}

void Write(JSONWriter& out, const aiScene& ai)
{
	out.StartObj();

	out.Key("version");
	WriteVersionInfo(out); //Check! 
	
	if(ai.HasMeshes()) {
		out.Key("meshes");
		out.StartArray();
		for(unsigned int n = 0; n < ai.mNumMeshes; ++n) {
			Write(out,*ai.mMeshes[n]);
		}
		out.EndArray();
	}
	
	//TODO: Take embedded textures out of model and into separate files
	
	if(ai.HasMaterials()) {
		std::set<aiString*> materialNames;
		out.Key("materials");
		out.StartArray();
		for(unsigned int n = 0; n < ai.mNumMaterials; ++n) {
			out.StartObj();
			std::array<aiString*, 4> values = Write(out,*ai.mMaterials[n],n);
			if (values[0] != nullptr) materialNames.insert(values[0]);
			if (values[1] != nullptr) materialNames.insert(values[1]);
			if (values[2] != nullptr) materialNames.insert(values[2]);
			if (values[3] != nullptr) materialNames.insert(values[3]);
			out.EndObj();
		}
		out.EndArray();
		
		out.Key("texture");
		out.StartArray();
		std::set<aiString*>::iterator iter = materialNames.begin();
		for (unsigned int i = 0; i < materialNames.size(); i++) {
			aiString* str = *iter++;
			assert(str != nullptr);
			out.StartObj();
			//Assign by index
			out.Key("id");
			out.SimpleValue(i);
			out.Key("filename");
			out.SimpleValue(str->C_Str());
		}
		out.EndArray();
	}
		
	out.Key("nodes");
	out.StartArray();
	Write(out,*ai.mRootNode,*ai.mMeshes, ai.mNumMeshes);
	out.EndArray();

	if(ai.HasAnimations()) {
		out.Key("animations");
		out.StartArray();
		for(unsigned int n = 0; n < ai.mNumAnimations; ++n) {
			Write(out,*ai.mAnimations[n]);
		}
		out.EndArray();
	}

	out.EndObj();
}


void Assimp2Libgdx(const char* file, Assimp::IOSystem* io, const aiScene* scene, const Assimp::ExportProperties*) 
{
	std::unique_ptr<Assimp::IOStream> str(io->Open(file,"wt"));
	assert(str != nullptr);
	
	// get a copy of the scene so we can modify it
	aiScene* scenecopy_tmp;
	aiCopyScene(scene, &scenecopy_tmp);

	try {
		// split meshes so they fit into a 16 bit signed index buffer
		MeshSplitter splitter;
		splitter.SetLimit(1 << 15);
		splitter.Execute(scenecopy_tmp);
		
		// XXX Flag_WriteSpecialFloats is turned on by default, right now we don't have a configuration interface for exporters
		JSONWriter s(*str,JSONWriter::Flag_WriteSpecialFloats);
		Write(s,*scenecopy_tmp);
	}
	catch(const std::exception &exc) {
		std::cerr << exc.what();
		aiFreeScene(scenecopy_tmp);
		throw;
	}
	aiFreeScene(scenecopy_tmp);
}

} // 
