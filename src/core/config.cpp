/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008, Buggy-Mushroom Studio
**
** Core functions implementation: config file operator
*/

#include "bsgl_impl.h"
#include <tinyxml2.h>

using namespace tinyxml2;

void CALL BSGL_Impl::Config_SetInt(const char* section, const char* option, int value) {
    char buf[256];

    if( !szCfgFile[0] ) {
        _PostError("The config file's path is unspecified.");
        return;
    }

    XMLDocument config_doc;
    config_doc.LoadFile(szCfgFile);

    XMLElement* section_node = config_doc.FirstChildElement(section);
    if( 0 == section_node ) {
        section_node = config_doc.NewElement(section);
        if( 0 == config_doc.InsertEndChild(section_node) ) {
            _PostError("Can't insert a section node.");
        }
    }

    XMLElement* option_node = section_node->FirstChildElement(option);
    if( 0 == option_node ) {
        option_node = config_doc.NewElement(option);
        if( 0 == section_node->InsertEndChild(option_node) ) {
            _PostError("Can't insert a option node.");
        }
    }

    sprintf(buf, "%d", value);
    option_node->SetText(buf);

    config_doc.SaveFile(szCfgFile);
}

int CALL BSGL_Impl::Config_GetInt(const char* section, const char* option, int def_val) {
    if( !szCfgFile[0] ) {
        return def_val;
    }

    XMLDocument config_doc;
    config_doc.LoadFile(szCfgFile);

    XMLElement* section_node = config_doc.FirstChildElement(section);
    if( section_node != 0 ) {
        XMLElement* option_node = section_node->FirstChildElement(option);
        if( option_node != 0 ) {
            char const* text = option_node->GetText();
            if( text != 0 ) {
                return atoi(text);
            }
        }
    }

    return def_val;
}

void CALL BSGL_Impl::Config_SetFloat(const char* section, const char* option, float value) {
    char buf[256];

    if( !szCfgFile[0] ) {
        _PostError("The config file's path is unspecified.");
        return;
    }

    XMLDocument config_doc;
    config_doc.LoadFile(szCfgFile);

    XMLElement* section_node = config_doc.FirstChildElement(section);
    if( 0 == section_node ) {
        section_node = config_doc.NewElement(section);
        if( 0 == config_doc.InsertEndChild(section_node) ) {
            _PostError("Can't insert a section node.");
        }
    }

    XMLElement* option_node = section_node->FirstChildElement(option);
    if( 0 == option_node ) {
        option_node = config_doc.NewElement(option);
        if( 0 == section_node->InsertEndChild(option_node) ) {
            _PostError("Can't insert a option node.");
        }
    }

    sprintf(buf, "%f", value);
    option_node->SetText(buf);

    config_doc.SaveFile(szCfgFile);
}

float CALL BSGL_Impl::Config_GetFloat(const char* section, const char* option, float def_val) {
    if( !szCfgFile[0] ) {
        return def_val;
    }

    XMLDocument config_doc;
    config_doc.LoadFile(szCfgFile);

    XMLElement* section_node = config_doc.FirstChildElement(section);
    if( section_node != 0 ) {
        XMLElement* option_node = section_node->FirstChildElement(option);
        if( option_node != 0 ) {
            char const* text = option_node->GetText();
            if( text != 0 ) {
                return (float)atof(text);
            }
        }
    }

    return def_val;
}

void CALL BSGL_Impl::Config_SetString(const char* section, const char* option, const char* value) {
    if( !szCfgFile[0] ) {
        _PostError("The config file's path is unspecified.");
        return;
    }

    XMLDocument config_doc;
    config_doc.LoadFile(szCfgFile);

    XMLElement* section_node = config_doc.FirstChildElement(section);
    if( 0 == section_node ) {
        section_node = config_doc.NewElement(section);
        if( 0 == config_doc.InsertEndChild(section_node) ) {
            _PostError("Can't insert a section node.");
        }
    }

    XMLElement* option_node = section_node->FirstChildElement(option);
    if( 0 == option_node ) {
        option_node = config_doc.NewElement(option);
        if( 0 == section_node->InsertEndChild(option_node) ) {
            _PostError("Can't insert a option node.");
        }
    }

    option_node->SetText(value);

    config_doc.SaveFile(szCfgFile);
}

char* CALL BSGL_Impl::Config_GetString(const char* section, const char* option, const char* def_val) {
    if( !szCfgFile[0] ) {
        strcpy(szCfgString, def_val);
        return szCfgString;
    }

    XMLDocument config_doc;
    config_doc.LoadFile(szCfgFile);

    XMLElement* section_node = config_doc.FirstChildElement(section);
    if( section_node != 0 ) {
        XMLElement* option_node = section_node->FirstChildElement(option);
        if( option_node != 0 ) {
            char const* text = option_node->GetText();
            if( text != 0 ) {
                strcpy(szCfgString, text);
                return szCfgString;
            }
        }
    }

    strcpy(szCfgString, def_val);
    return szCfgString;
}
