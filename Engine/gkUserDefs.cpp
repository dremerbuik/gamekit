/*
-------------------------------------------------------------------------------
    This file is part of OgreKit.
    http://gamekit.googlecode.com/

    Copyright (c) 2006-2010 Charlie C.

    Contributor(s): silveira.nestor.
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
#include "OgreConfigFile.h"
#include "OgreStringConverter.h"

#include "gkUserDefs.h"
#include "gkLogger.h"
#include "gkPath.h"
#include "gkUtils.h"
#include "gkPath.h"


// ----------------------------------------------------------------------------
gkUserDefs::gkUserDefs() 
    :   rendersystem(OGRE_RS_GL),
        log("OgreKit.log"),
        verbose(true),
        winsize(800, 600),
        wintitle("Ogre GameKit Demo"),
        fullscreen(false),
        resources(""),
        animspeed(25),
        startframe(1),
        blendermat(false),
        grabInput(true),
        debugPhysics(false),
        debugPhysicsAabb(false),
        enableshadows(false),
        buildInstances(false),
        useBulletDbvt(true),
        showDebugProps(false),
        shadowtechnique("none"),
        colourshadow(0, 0, 0),
        fardistanceshadow(0)
{
}

// ----------------------------------------------------------------------------
void gkUserDefs::load(const gkString &fname)
{
    gkString startup;

    gkPath pth(fname);
    if (pth.isFile())
        startup = pth.getPath();

    if (startup.empty())
        return;


    try
    {
        // try and load prefs

        Ogre::ConfigFile fp;
        fp.load(startup);

        Ogre::ConfigFile::SectionIterator cit = fp.getSectionIterator();
        while (cit.hasMoreElements())
        {
            Ogre::ConfigFile::SettingsMultiMap *ptr = cit.getNext();
            for (Ogre::ConfigFile::SettingsMultiMap::iterator dit = ptr->begin(); dit != ptr->end(); ++dit)
            {
                gkString key = dit->first;
                gkString val = dit->second;

                // not case sensitive
                Ogre::StringUtil::toLowerCase(key);
                parseString(key, val);
            }
        }
    }
    catch (Ogre::Exception &e)
    {
        gkLogMessage("Failed to load resource file!\n" << e.getDescription());
    }
}


// ----------------------------------------------------------------------------
void gkUserDefs::parseString(const gkString &key, const gkString &val)
{
#define KeyEq(b) (key == b)

    if (KeyEq("rendersystem"))
    {
        rendersystem = OGRE_RS_GL;


        if (val.find("d3d9") != val.npos)
            rendersystem = OGRE_RS_D3D9;
        if (val.find("d3d10") != val.npos)
            rendersystem = OGRE_RS_D3D10;
        if (val.find("d3d11") != val.npos)
            rendersystem = OGRE_RS_D3D11;

        return;
    }

    if (KeyEq("log"))
    {
        log = val;
        return;
    }


    if (KeyEq("debugphysics"))
    {
        debugPhysics = Ogre::StringConverter::parseBool(val);
        return;
    }
    
    
    if (KeyEq("debugphysicsaabb"))
    {
        debugPhysicsAabb = Ogre::StringConverter::parseBool(val);
        return;
    }

    if (KeyEq("grabinput"))
    {
        grabInput = Ogre::StringConverter::parseBool(val);
        return;
    }
    if (KeyEq("verbose"))
    {
        verbose = Ogre::StringConverter::parseBool(val);
        return;
    }

    if (KeyEq("winsize"))
    {
        winsize = Ogre::StringConverter::parseVector2(val);
        return;
    }

    if (KeyEq("wintitle"))
    {
        wintitle = val;
        return;
    }
    if (KeyEq("fullscreen"))
    {
        fullscreen = Ogre::StringConverter::parseBool(val);
        return;
    }
    if (KeyEq("resources"))
    {
        gkPath p(val);
        if (p.isFile())
            resources = val;

        return;
    }
    if (KeyEq("animspeed"))
    {
        animspeed = Ogre::StringConverter::parseReal(val);
        animspeed = gkMaxf(animspeed, 1.0);
        return;
    }
    if (KeyEq("startframe"))
    {
        startframe = Ogre::StringConverter::parseReal(val);
        startframe = gkMaxf(startframe, 1.0);
        return;
    }
    if (KeyEq("blendermat"))
    {
        blendermat = Ogre::StringConverter::parseBool(val);
        return;
    }
    if (KeyEq("buildinstances"))
    {
        buildInstances = Ogre::StringConverter::parseBool(val);
        return;
    }
    if (KeyEq("frustumculling"))
    {
        useBulletDbvt = Ogre::StringConverter::parseBool(val);
        return;
    }
    if (KeyEq("showdebugprops"))
    {
        showDebugProps = Ogre::StringConverter::parseBool(val);
        return;
    }
    if (KeyEq("fullscreen"))
    {
        fullscreen = Ogre::StringConverter::parseBool(val);
        return;
    }
    if (KeyEq("enableshadows"))
    {
        enableshadows = Ogre::StringConverter::parseBool(val);
        return;
    }
    if (KeyEq("shadowtechnique"))
    {
        shadowtechnique = val;
        return;
    }
    if (KeyEq("colourshadow"))
    {
        colourshadow = Ogre::StringConverter::parseColourValue(val);
        return;
    }
    if (KeyEq("fardistanceshadow"))
    {
        fardistanceshadow = Ogre::StringConverter::parseReal(val);
        return;
    }
}