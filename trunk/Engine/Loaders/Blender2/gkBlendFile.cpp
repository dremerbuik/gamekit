/*
-------------------------------------------------------------------------------
    This file is part of OgreKit.
    http://gamekit.googlecode.com/

    Copyright (c) 2006-2010 Charlie C.

    Contributor(s): Nestor Silveira.
-------------------------------------------------------------------------------
  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------------
*/
#include "OgreException.h"
#include "OgreTexture.h"
#include "OgreTextureManager.h"
#include "OgreMaterialManager.h"
#include "OgreParticleSystemManager.h"

#include "bBlenderFile.h"
#include "bMain.h"
#include "Blender.h"


#include "gkBlendFile.h"
#include "gkBlendLoader.h"
#include "gkSceneManager.h"
#include "gkScene.h"
#include "gkGameObject.h"

#include "gkBlenderDefines.h"
#include "gkBlenderSceneConverter.h"
#include "gkTextureLoader.h"
#include "gkPath.h"
#include "gkUtils.h"
#include "gkLogger.h"


#include "gkTextManager.h"
#include "gkTextFile.h"
#include "gkEngine.h"
#include "gkUserDefs.h"
#include "Script/Lua/gkLuaManager.h"

#ifdef OGREKIT_OPENAL_SOUND
# include "Sound/gkSoundManager.h"
# include "Sound/gkSound.h"
#endif

using namespace Ogre;


// ----------------------------------------------------------------------------
gkBlendFile::gkBlendFile(const gkString &blendToLoad, const gkString &group)
	:	m_name(blendToLoad),
	    m_group(group),
		m_activeScene(0)
{
}


// ----------------------------------------------------------------------------
gkBlendFile::~gkBlendFile()
{
	if (!m_loaders.empty())
	{
		ManualResourceLoaderList::Iterator it = m_loaders.iterator();
		while (it.hasMoreElements())
			delete it.getNext();
	}
}




// ----------------------------------------------------------------------------
bool gkBlendFile::parse(int opts)
{

	utMemoryStream fs;
	fs.open(m_name.c_str(), utStream::SM_READ);

	if (!fs.isOpen())
	{
		gkLogMessage("BlendFile: File " << m_name << " loading failed. No such file.");
		return false;
	}

	// Write contents and inflate.
	utMemoryStream buffer;
	fs.inflate(buffer);

	m_file = new bParse::bBlenderFile((char*)buffer.ptr(), buffer.size());
	m_file->parse(false);

	if (!m_file->ok())
	{
		gkLogMessage("BlendFile: File " << m_name << " loading failed. Data error.");
		return false;
	}


	doVersionTests();


	if (opts == gkBlendLoader::LO_ONLY_ACTIVE_SCENE)
		loadActive();
	else 
		initializeAll();

	delete m_file;
	m_file = 0;
	return true;
}



// ----------------------------------------------------------------------------
void gkBlendFile::loadActive(void)
{
	// Load / convert only the active scene.

	Blender::FileGlobal *fg = (Blender::FileGlobal *)m_file->getFileGlobal();
	if (fg)
	{
		buildAllTextures();
		buildTextFiles();
		buildAllSounds();

		// parse & build
		Blender::Scene *sc = (Blender::Scene *)fg->curscene;
		if (sc != 0)
		{
			gkBlenderSceneConverter conv(this, sc);
			conv.convert();

			m_activeScene = gkSceneManager::getSingleton().getScene(GKB_IDNAME(sc));
			if (m_activeScene)
				m_scenes.push_back(m_activeScene);
		}
	}

}


// ----------------------------------------------------------------------------
void gkBlendFile::initializeAll(void)
{
	// Load / convert all 
	buildAllTextures();
	buildTextFiles();
	buildAllSounds();

	
	bParse::bListBasePtr *scenes = m_file->getMain()->getScene();
	int i;
	for (i =0; i<scenes->size(); ++i)
	{
		Blender::Scene *sc = (Blender::Scene *)scenes->at(i);

		if (sc != 0)
		{
			gkBlenderSceneConverter conv(this, sc);
			conv.convert();

			gkScene *gks = gkSceneManager::getSingleton().getScene(GKB_IDNAME(sc));
			if (gks)
				m_scenes.push_back(gks);
		}
	}


	Blender::FileGlobal *fg = (Blender::FileGlobal *)m_file->getFileGlobal();
	if (fg)
	{
		// Grab the main scene
		Blender::Scene *sc = (Blender::Scene *)fg->curscene;
		if (sc != 0)
			m_activeScene = gkSceneManager::getSingleton().getScene(GKB_IDNAME(sc));
	}

	if (m_activeScene == 0 && !m_scenes.empty())
		m_activeScene = m_scenes.front();
}




// ----------------------------------------------------------------------------
gkScene *gkBlendFile::getSceneByName(const gkString &name)
{

	Scenes::Iterator it = m_scenes.iterator();
	while (it.hasMoreElements())
	{
		gkScene *ob = it.getNext();
		if (ob->getName() == name)
			return ob;
	}
	return 0;
}



// ----------------------------------------------------------------------------
void gkBlendFile::buildTextFiles(void)
{
	// Create a list of all internal text blocks

	bParse::bMain *mp = m_file->getMain();
	bParse::bListBasePtr *text = mp->getText();


	gkTextManager &txtMgr = gkTextManager::getSingleton();

	for (int i=0; i<text->size(); ++i)
	{
		Blender::Text *txt = (Blender::Text *)text->at(i);
		Blender::TextLine *tl = (Blender::TextLine *)txt->lines.first;

		std::stringstream ss;
		while (tl)
		{
			tl->line[tl->len] = 0;

			ss << tl->line << '\n';
			tl = tl->next;
		}

		// could be optimized
		gkString str = ss.str();
		if (!str.empty() && !txtMgr.hasFile(GKB_IDNAME(txt)))
		{
			gkTextFile *tf = txtMgr.create(GKB_IDNAME(txt));
			tf->setText(str);

			if(tf->getName().find(".material") != gkString::npos)
			{
				DataStreamPtr memStream(
				    OGRE_NEW MemoryDataStream((void *)str.c_str(), str.size()));

				Ogre::MaterialManager::getSingleton().parseScript(memStream, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			}
			else if(tf->getName().find(".particle") != gkString::npos)
			{
				DataStreamPtr memStream(
				    OGRE_NEW MemoryDataStream((void *)str.c_str(), str.size()));

				Ogre::ParticleSystemManager::getSingleton().parseScript(memStream, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			}
			else if(tf->getName().find(".lua") != gkString::npos)
				gkLuaManager::getSingleton().create(GKB_IDNAME(txt), str);
		}
	}
}


// ----------------------------------------------------------------------------
void gkBlendFile::buildAllTextures(void)
{
	bParse::bListBasePtr *imaPtr = m_file->getMain()->getImage();

	int i;
	for (i=0; i<imaPtr->size(); ++i)
	{
		Blender::Image *ima = (Blender::Image *)imaPtr->at(i);

		// don't try & convert zero users
		if (ima->id.us <= 0)
			continue;


		Ogre::TexturePtr tex = Ogre::TextureManager::getSingleton().getByName(GKB_IDNAME(ima));
		if (!tex.isNull())
			continue;


		gkTextureLoader *loader = new gkTextureLoader(ima);
		tex = Ogre::TextureManager::getSingleton().create(GKB_IDNAME(ima), m_group, true, loader);

		if (!tex.isNull())
			m_loaders.push_back(loader);
		else
			delete loader;
	}
}


// ----------------------------------------------------------------------------
void gkBlendFile::buildAllSounds()
{
#ifdef OGREKIT_OPENAL_SOUND

	bParse::bMain *mp = m_file->getMain();

	bParse::bListBasePtr *soundList = mp->getSound();
	gkSoundManager *mgr = gkSoundManager::getSingletonPtr();

	if (!mgr->isValidContext())
		return;

	for (int i=0; i<soundList->size(); ++i)
	{
		Blender::bSound *sound = (Blender::bSound *)soundList->at(i);

		// skip zero users
		if (sound->id.us <= 0)
			continue;


		gkPath pth(sound->name);
		bool isFile = pth.isFile();

		if (sound->packedfile || sound->newpackedfile || isFile)
		{
			Blender::PackedFile *pak = sound->packedfile ? sound->packedfile : sound->newpackedfile;
			if (((pak && pak->data) || isFile) && !mgr->hasSound(GKB_IDNAME(sound)))
			{
				gkSound *sndObj = mgr->createSound(GKB_IDNAME(sound));
				if (!sndObj)
					continue;

				if (isFile)
				{
					// Attempt to stream from file
					if (!sndObj->load(pth.getPath().c_str()))
					{
						mgr->destroy(sndObj);
						continue;
					}

					gkLogMessage("Sound: Loaded file " << pth.getPath() << " as" << GKB_IDNAME(sound) << ".");
				}
				else if (pak && pak->data)
				{
					// initialize from buffer
					if (!sndObj->load(pak->data, pak->size))
					{
						mgr->destroy(sndObj);
						continue;
					}

					gkLogMessage("Sound: Loaded buffer " << GKB_IDNAME(sound) << ".");
				}
				else
					GK_ASSERT(0);
			}
		}
	}

#endif
}


// ----------------------------------------------------------------------------
void gkBlendFile::doVersionTests(void)
{
	bParse::bMain *main = m_file->getMain();
	int version = main->getVersion();

	bParse::bListBasePtr *iter;
	int i, s;

	if (version <= 242)
	{
		Blender::FileGlobal *fg = (Blender::FileGlobal *)m_file->getFileGlobal();

		if (fg)
		{
			if (!fg->curscene)
				fg->curscene = (Blender::Scene *)main->getScene()->at(0);
		}
	}

	if (version <= 249)
	{
		iter = main->getObject();
		i=0;
		s =iter->size();

		while (i < s)
		{
			Blender::Object *ob = (Blender::Object *)iter->at(i++);

			if (ob->gameflag & OB_DYNAMIC)
				ob->body_type = ob->gameflag & OB_RIGID_BODY ? OB_BODY_TYPE_RIGID : OB_BODY_TYPE_DYNAMIC;
			else if (ob->gameflag & OB_RIGID_BODY)
				ob->body_type = OB_BODY_TYPE_RIGID;
			else
				ob->body_type = OB_BODY_TYPE_STATIC;
		}
	}

	if (version <= 250)
	{
		iter = main->getObject();
		i=0;
		s =iter->size();
		while (i < s)
		{
			Blender::Object *bobj = (Blender::Object *)iter->at(i++);

			for (Blender::bConstraint *bc = (Blender::bConstraint *)bobj->constraints.first; bc; bc = bc->next)
			{
				// convert rotation types to radians
				if (bc->type == CONSTRAINT_TYPE_ROTLIMIT)
				{
					Blender::bRotLimitConstraint *lr = (Blender::bRotLimitConstraint *)bc->data;
					lr->xmax *= gkRPD;
					lr->xmin *= gkRPD;
					lr->ymax *= gkRPD;
					lr->ymin *= gkRPD;
					lr->zmax *= gkRPD;
					lr->zmin *= gkRPD;
				}
			}
		}
	}
}