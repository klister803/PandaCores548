/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Config.h"
#include "Log.h"
#include "Util.h"
#include <ace/Auto_Ptr.h>
#include <ace/Configuration_Import_Export.h>
#include <ace/Thread_Mutex.h>
#include <boost/property_tree/ini_parser.hpp>
#include <algorithm>
#include <memory>

namespace ConfigMgr
{

namespace bpt = boost::property_tree;

namespace
{
    typedef ACE_Thread_Mutex LockType;
    typedef ACE_Guard<LockType> GuardType;

    std::string _filename;
    bpt::ptree _config;
    LockType m_configLock;

    // Defined here as it must not be exposed to end-users.
    template<class T>
    T GetValueHelper(const char* name, T def)
    {
        GuardType guard(m_configLock);

        try
        {
            return _config.get<T>(bpt::ptree::path_type(name, '/'));
        }
        catch (bpt::ptree_bad_path)
        {
            printf("Missing name %s in config file %s, add \"%s\" to this file", name, _filename.c_str(), name);
        }
        catch (bpt::ptree_bad_data)
        {
            printf("Bad value defined for name %s in config file %s", name, _filename.c_str());
        }

        return def;
    }
}

bool Load(const char* file)
{
    GuardType guard(m_configLock);

    if (file)
        _filename = file;

    try
    {
        bpt::ptree fullTree;
        bpt::ini_parser::read_ini(_filename, fullTree);

        if (fullTree.empty())
        {
            printf("ConfigMgr::Load fullTree.empty() may be dubplicate name in option config");
            return false;
        }

        // Since we're using only one section per config file, we skip the section and have direct property access
        _config = fullTree.begin()->second;
    }
    catch (std::exception& e)
    {
        printf("ConfigMgr::Load exception %s", e.what());
        return false;
    }

    return true;
}

std::string GetStringDefault(std::string const& name, const std::string &def)
{
    return GetStringDefault(name.c_str(), def);
};

std::string GetStringDefault(const char* name, const std::string &def)
{
    std::string val = GetValueHelper(name, def);
    val.erase(std::remove(val.begin(), val.end(), '"'), val.end());
    return val;
};

bool GetBoolDefault(const char* name, bool def)
{
    std::string val = GetValueHelper(name, std::string(def ? "1" : "0"));
    val.erase(std::remove(val.begin(), val.end(), '"'), val.end());
    return StringToBool(val);
};

int GetIntDefault(const char* name, int def)
{
    return GetValueHelper(name, def);
};

float GetFloatDefault(const char* name, float def)
{
    return GetValueHelper(name, def);
};

const std::string & GetFilename()
{
    GuardType guard(m_configLock);
    return _filename;
}

std::vector<std::string> GetKeysByString(std::string const& name)
{
    GuardType guard(m_configLock);

    std::vector<std::string> keys;

    for (bpt::ptree::value_type const& child : _config)
        if (child.first.compare(0, name.length(), name) == 0)
            keys.push_back(child.first);

    return keys;
}

} // namespace
