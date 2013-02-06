/*
** Buggy-Mushroom's Spore Game Library
** Copyright (C) 2008, Buggy-Mushroom Studio
**
** Core functions implementation: config file operator
*/

#include "bsgl_impl.h"
#include "tinyxml.h"

void CALL BSGL_Impl::Config_SetInt(const char* section, const char* option, int value) {
    char buf[256];

    if( !szCfgFile[0] ) {
        _PostError("The config file's path is unspecified.");
        return;
    }

    TiXmlDocument config_doc;
    config_doc.LoadFile(szCfgFile);

    TiXmlNode* section_node = config_doc.FirstChild(section);
    if( 0 == section_node ) {
        TiXmlElement new_element(section);
        section_node = config_doc.InsertEndChild(new_element);
        if( 0 == section_node ) {
            _PostError("Can't insert a section node.");
        }
    }

    TiXmlNode* option_node = section_node->FirstChild(option);
    if( 0 == option_node ) {
        TiXmlElement new_element(option);
        option_node = section_node->InsertEndChild(new_element);
        if( 0 == option_node ) {
            _PostError("Can't insert a option node.");
        }
        TiXmlText nulltext("");
        option_node->InsertEndChild(nulltext);
    }

    TiXmlNode* node = option_node->FirstChild();
    if( node != 0 ) {
        sprintf(buf, "%d", value);
        TiXmlText text(buf);
        option_node->ReplaceChild(node, text);
    }

    config_doc.SaveFile();
}

int CALL BSGL_Impl::Config_GetInt(const char* section, const char* option, int def_val) {
    if( !szCfgFile[0] ) {
        return def_val;
    }

    TiXmlDocument config_doc;
    config_doc.LoadFile(szCfgFile);

    TiXmlNode* section_node = config_doc.FirstChild(section);
    if( section_node != 0 ) {
        TiXmlNode* option_node = section_node->FirstChild(option);
        if( option_node != 0 ) {
            TiXmlNode* node = option_node->FirstChild();
            if( node != 0 ) {
                TiXmlText* text = node->ToText();
                if( text != 0 ) {
                    return atoi(text->Value());
                }
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

    TiXmlDocument config_doc;
    config_doc.LoadFile(szCfgFile);

    TiXmlNode* section_node = config_doc.FirstChild(section);
    if( 0 == section_node ) {
        TiXmlElement new_element(section);
        section_node = config_doc.InsertEndChild(new_element);
        if( 0 == section_node ) {
            _PostError("Can't insert a section node.");
        }
    }

    TiXmlNode* option_node = section_node->FirstChild(option);
    if( 0 == option_node ) {
        TiXmlElement new_element(option);
        option_node = section_node->InsertEndChild(new_element);
        if( 0 == option_node ) {
            _PostError("Can't insert a option node.");
        }
        TiXmlText nulltext("");
        option_node->InsertEndChild(nulltext);
    }

    TiXmlNode* node = option_node->FirstChild();
    if( node != 0 ) {
        sprintf(buf, "%f", value);
        TiXmlText text(buf);
        option_node->ReplaceChild(node, text);
    }

    config_doc.SaveFile();
}

float CALL BSGL_Impl::Config_GetFloat(const char* section, const char* option, float def_val) {
    if( !szCfgFile[0] ) {
        return def_val;
    }

    TiXmlDocument config_doc;
    config_doc.LoadFile(szCfgFile);

    TiXmlNode* section_node = config_doc.FirstChild(section);
    if( section_node != 0 ) {
        TiXmlNode* option_node = section_node->FirstChild(option);
        if( option_node != 0 ) {
            TiXmlNode* node = option_node->FirstChild();
            if( node != 0 ) {
                TiXmlText* text = node->ToText();
                if( text != 0 ) {
                    return (float)atof(text->Value());
                }
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

    TiXmlDocument config_doc;
    config_doc.LoadFile(szCfgFile);

    TiXmlNode* section_node = config_doc.FirstChild(section);
    if( 0 == section_node ) {
        TiXmlElement new_element(section);
        section_node = config_doc.InsertEndChild(new_element);
        if( 0 == section_node ) {
            _PostError("Can't insert a section node.");
        }
    }

    TiXmlNode* option_node = section_node->FirstChild(option);
    if( 0 == option_node ) {
        TiXmlElement new_element(option);
        option_node = section_node->InsertEndChild(new_element);
        if( 0 == option_node ) {
            _PostError("Can't insert a option node.");
        }
        TiXmlText nulltext("");
        option_node->InsertEndChild(nulltext);
    }

    TiXmlNode* node = option_node->FirstChild();
    if( node != 0 ) {
        TiXmlText text(value);
        option_node->ReplaceChild(node, text);
    }

    config_doc.SaveFile();
}

char* CALL BSGL_Impl::Config_GetString(const char* section, const char* option, const char* def_val) {
    if( !szCfgFile[0] ) {
        strcpy(szCfgString, def_val);
        return szCfgString;
    }

    TiXmlDocument config_doc;
    config_doc.LoadFile(szCfgFile);

    TiXmlNode* section_node = config_doc.FirstChild(section);
    if( section_node != 0 ) {
        TiXmlNode* option_node = section_node->FirstChild(option);
        if( option_node != 0 ) {
            TiXmlNode* node = option_node->FirstChild();
            if( node != 0 ) {
                TiXmlText* text = node->ToText();
                if( text != 0 ) {
                    strcpy(szCfgString, text->Value());
                    return szCfgString;
                }
            }
        }
    }

    strcpy(szCfgString, def_val);
    return szCfgString;
}

