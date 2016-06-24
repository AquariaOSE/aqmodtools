#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tinyxml2.h"

#include <sstream>
#include <vector>
#include <algorithm>
#include <list>
#include <fstream>
#include <map>

using namespace tinyxml2;

enum Operation
{
	AOP_COPY_STRIPS, // srcidx, dstidx
	AOP_CLEAN_FILE,  // remove data of non-existing bones
	AOP_REMOVE_BONEDATA, // remove data of one bone and set it to defaults in all anims - (0, 0) and 0 rotation

};

XMLDocument doc;

std::vector<XMLElement*> bonesv;
std::map<std::string, XMLElement*> animsv;


static void fail(const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	fputc('\n', stderr);
	va_end(va);
	exit(1);
}

static void warn(const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	fprintf(stderr, "Warning: ");
	vfprintf(stderr, fmt, va);
	fputc('\n', stderr);
	va_end(va);
}

static void loadXML(const char *fn)
{
	XMLError err = doc.LoadFile(fn);
	if(err != XML_NO_ERROR)
		fail("Error %d opening XML file", err);

	XMLElement *bones = doc.FirstChildElement("Bones");
	if(!bones)
		fail("<Bones> tag missing");

	XMLElement *animations = doc.FirstChildElement("Animations");
	if(!animations)
		fail("<Animations> tag missing");

	for(XMLElement *bone = bones->FirstChildElement("Bone"); bone; bone = bone->NextSiblingElement("Bone"))
		bonesv.push_back(bone);

	for(XMLElement *anim = animations->FirstChildElement("Animation"); anim; anim = anim->NextSiblingElement("Animation"))
	{
		const char *name = anim->Attribute("name");
		if(name)
			animsv[name] = anim;
		else
			warn("Animation has no name!");
	}

#ifdef _DEBUG
	printf("Loaded %s\n", fn);
#endif
}

XMLElement *getBoneByIdx(int idx)
{
	for(size_t i = 0; i < bonesv.size(); ++i)
		if(XMLElement *b = bonesv[i])
		{
			int bidx;
			if(b->QueryIntAttribute("idx", &bidx) == XML_NO_ERROR)
				if(bidx == idx)
					return b;
		}
	return NULL;
}

struct Vec2
{
	Vec2() : x(0), y(0) {}
	Vec2(float x_, float y_) : x(x_), y(y_) {}
	float x, y;
};

struct BoneKeyframe
{
	int idx;
	float x, y, rot;
	std::vector<Vec2> strips;
};

struct KeyframeData
{
	float time;
	std::vector<BoneKeyframe> b;

	std::string getDataString() const
	{
		std::ostringstream os;
		os << time << " ";
		for(size_t i = 0; i < b.size(); ++i)
		{
			const BoneKeyframe& bk = b[i];
			os << bk.idx << " "<< bk.x << " " << bk.y << " " << bk.rot << " " << bk.strips.size() << " ";
			for(size_t j = 0; j < bk.strips.size(); ++j)
				os << bk.strips[j].x << " " << bk.strips[j].y << " ";
		}
		return os.str();
	}

	BoneKeyframe *getBoneByIdx(int idx)
	{
		for(size_t i = 0; i < b.size(); ++i)
			if(b[i].idx == idx)
				return &b[i];
		return NULL;
	}
};

void parseDataString(const char *e, KeyframeData& kf)
{
	std::istringstream is(e);
	int idx;
	is >> kf.time;
	while(is >> idx)
	{
		int strip;
		BoneKeyframe bkf;
		bkf.idx = idx;
		is >> bkf.x >> bkf.y >> bkf.rot >> strip;
		bkf.strips.reserve(strip);
		for(int i = 0; i < strip; ++i)
		{
			Vec2 v;
			is >> v.x >> v.y;
			bkf.strips.push_back(v);
		}
		kf.b.push_back(bkf);
	}
}

void parseKeyData(XMLElement *key, KeyframeData& kf)
{
	const char *e = key->Attribute("e");
	if(e)
		parseDataString(e, kf);
	else
		fail("Missing 'e' attribute");
}

template<typename T>
void forEachKeyframe(T& f)
{
	for(std::map<std::string, XMLElement*>::iterator it = animsv.begin(); it != animsv.end(); ++it)
	{
		for(XMLElement *key = it->second->FirstChildElement("Key"); key; key = key->NextSiblingElement("Key"))
			f(key);
	}
}

template<typename T>
void forEachKeyframeInAnim(T& f, const std::string& animname)
{
	std::map<std::string, XMLElement*>::iterator it = animsv.find(animname);
	if(it != animsv.end())
	{
		for(XMLElement *key = it->second->FirstChildElement("Key"); key; key = key->NextSiblingElement("Key"))
			f(key);
	}
	else
		warn("Animation not found: %s", animname.c_str());
}

class KeyframeModifier
{
public:
	void operator() (XMLElement *key)
	{
		KeyframeData kf;
		parseKeyData(key, kf);
		doit(kf);
		std::string ds = kf.getDataString();
		key->SetAttribute("e", ds.c_str());
	}
protected:
	virtual void doit(KeyframeData &kf) = 0;
};

class CopyStrips : public KeyframeModifier
{
public:
	CopyStrips(int src, int dst) : srcidx(src), dstidx(dst) {}
protected:
	void doit(KeyframeData &kf)
	{
		kf.getBoneByIdx(dstidx)->strips = kf.getBoneByIdx(srcidx)->strips;
	}
	int srcidx, dstidx;
};

class ResetStrips : public KeyframeModifier
{
public:
	ResetStrips(int i) : idx(i) {}
protected:
	void doit(KeyframeData &kf)
	{
		std::vector<Vec2>& strips = kf.getBoneByIdx(idx)->strips;
		Vec2 z(0, 0);
		for(size_t i = 0; i < strips.size(); ++i)
			strips[i] = z;
	}
	int idx;
};

class DeleteStrips : public KeyframeModifier
{
public:
	DeleteStrips(int i) : idx(i) {}
protected:
	void doit(KeyframeData &kf)
	{
		kf.getBoneByIdx(idx)->strips.clear();
	}
	int idx;
};


void cmd_copystrips(std::istringstream& is)
{
	int srcidx, dstidx;
	if(!(is >> srcidx >> dstidx))
		fail("copystrips: not enough params");

	XMLElement *srcbone = getBoneByIdx(srcidx);
	XMLElement *dstbone = getBoneByIdx(dstidx);
	const char *srcattr = srcbone->Attribute("strip");
	if(!srcattr)
	{
		warn("copystrips: Bone %u does not have strips, ignored. Use 'delstrips' to remove strips data from a bone");
		return;
	}

	dstbone->SetAttribute("strip", srcattr);

	CopyStrips cs(srcidx, dstidx);
	forEachKeyframe(cs);
}

void cmd_animresetstrips(std::istringstream& is)
{
	int idx;
	std::string animname;
	if(!(is >> animname >> idx))
		fail("animresetstrips: not enough params");
	ResetStrips rs(idx);
	forEachKeyframeInAnim(rs, animname);
}

void cmd_delstrips(std::istringstream& is)
{
	int idx;
	while(is >> idx)
	{
		DeleteStrips ds(idx);
		forEachKeyframe(ds);
		XMLElement *bone = getBoneByIdx(idx);
		bone->DeleteAttribute("strip");
	}
}

struct CmdHandler
{
	const char *name;
	void (*func)(std::istringstream& is);
};

CmdHandler cmdhs[] =
{
	{ "copystrips", cmd_copystrips },
	{ "animresetstrips", cmd_animresetstrips },
	{ "delstrips", cmd_delstrips },
	{ NULL, NULL },
};

bool saveXML(const char *fn)
{
	return doc.SaveFile(fn) == XML_NO_ERROR;
}

void parseCmd(const char *s)
{
#ifdef _DEBUG
	puts(s);
#endif
	std::istringstream is(s);
	std::string cmd;
	is >> cmd;
	for(unsigned i = 0; cmdhs[i].name; ++i)
		if(!strcmp(cmd.c_str(), cmdhs[i].name))
		{
			cmdhs[i].func(is);
			if(!is.eof())
				warn("Ignored extra data in line: %s", is.str().c_str());
			return;
		}

	fail("Unknown cmd: %s", s);
}

int main(int argc, char **argv)
{
	std::string infn, outfn;
	std::vector<std::string> commands;
	if(argc < 3)
	{
		puts("animtool <commands...> -i <in.xml> [-o <out.xml>]");
		return 2;
	}

	for(int i = 1; i < argc; ++i)
	{
		const char *arg = argv[i];
		if(arg[0] == '-')
		{
			if(i + 1 >= argc)
				fail("Incomplete param");

			if(!strcmp(arg+1, "i"))
			{
				infn = argv[++i];
				continue;
			}
			if(!strcmp(arg+1, "o"))
			{
				outfn = argv[++i];
				continue;
			}

			fail("Unknown switch: %s", arg);
		}
		else
			commands.push_back(arg);
	}

	if(outfn.empty())
		outfn = infn;

	loadXML(infn.c_str());

	for(size_t i = 0; i < commands.size(); ++i)
		parseCmd(commands[i].c_str());

	saveXML(outfn.c_str());

	return 0;
}
