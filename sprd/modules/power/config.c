/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

#define LOG_TAG "PowerHAL"
#include <utils/Log.h>

#include "config.h"

struct power power;

/**
 * get_node_set - get all nodes by path
 * @doc: from the xmlParseFile() function
 * @xpath: the node path
 * return: include the nodes information
 */
static xmlXPathObjectPtr get_node_set(xmlDocPtr doc, xmlChar *xpath)
{
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;
    context = xmlXPathNewContext(doc);
    if(context == NULL)
    {
        ALOGE("Error in xmlXPathNewContent\n");
        return NULL;
    }
    result = xmlXPathEvalExpression(xpath, context);
    xmlXPathFreeContext(context);
    if(result == NULL)
    {
        ALOGE("Error in xmlXPathEvalExpression\n");
        return NULL;
    }
    if(xmlXPathNodeSetIsEmpty(result->nodesetval))
    {
        xmlXPathFreeObject(result);
        ALOGE("No result\n");
        return NULL;
    }
    return result;
}

/**
 * Parse scene node
 */
static int parse_scene_node(xmlNodePtr cur, struct scene *scene)
{
    int ret = 1;
    xmlChar *path, *file, *value, *name;
    struct set *set = NULL;

    name = xmlGetProp(cur, (const xmlChar*) "name");
    strncpy(scene->name, (const char *)name, LEN_SCENE_NAME_MAX);
    ALOGD_IF(DEBUG_V, "  <%s name=\"%s\" />", cur->name, name);
    xmlFree(name);

    cur = cur->xmlChildrenNode;
    while (cur) {
        if (xmlStrcmp(cur->name, (const xmlChar*) "set") == 0) {
            if (scene->count >= NUM_FILE_MAX) {
                ALOGE("!!!The sets[] if full");
                return 0;
            }

            set = &(scene->sets[scene->count++]);
            path = xmlGetProp(cur, (const xmlChar*) "path");
            file = xmlGetProp(cur, (const xmlChar*) "file");
            value = xmlGetProp(cur, (const xmlChar*) "value");
            ALOGD_IF(DEBUG_V, "    <%s path=\"%s\" file=\"%s\" value=\"%s\" />", cur->name, path, file, value);
            if (path == NULL || file == NULL || value == NULL) {
                if (path != NULL) xmlFree(path);
                if (file != NULL) xmlFree(file);
                if (value !=NULL) xmlFree(value);
                return 0;
            }
            strncpy(set->path, (const char *)path, LEN_PATH_MAX);
            if (set->path[strlen(set->path) - 1] == '/')
                set->path[strlen(set->path) - 1] = '\0';
            strncpy(set->file, (const char *)file, LEN_FILE_MAX);
            strncpy(set->value, (const char *)value, LEN_VALUE_MAX);

            xmlFree(path);
            xmlFree(file);
            xmlFree(value);
        }
        cur = cur->next;
    }

    ALOGD_IF(DEBUG_V, "  </scene>");
    return 1;
}

/**
 * Parse a mode node
 */
static int parse_mode_node(xmlNodePtr cur)
{
    xmlChar *name;
    struct scene *scene = NULL;
    struct mode *mode = NULL;

    if (CC_UNLIKELY(power.count >= NUM_MODE_MAX)) {
        ALOGE("!!!The modes[] is full");
        return 0;
    }

    mode = &(power.modes[power.count++]);
    name = xmlGetProp(cur, (const xmlChar*) "name");
    strncpy(mode->name, (const char *)name, LEN_MODE_NAME_MAX);
    ALOGD_IF(DEBUG_V, "<mode name=\"%s\" >", name);
    xmlFree(name);

    cur = cur->xmlChildrenNode;
    while (cur) {
        if (strncmp("scene", (const char *)cur->name, 5) == 0) {
            if (CC_UNLIKELY(mode->count >= NUM_SCENE_MAX)) {
                ALOGE("!!!The scenes[] is full");
                return 0;
            }
            scene = &(mode->scenes[mode->count++]);
            if (parse_scene_node(cur, scene) == 0) {
                return 0;
            }
        }

        cur = cur->next;
    }

    ALOGD_IF(DEBUG_V, "</mode>");
    return 1;
}

/**
 * Parse the scene config file
 */
int read_scene_config(void)
{
    int ret = 1;
    xmlChar *xpath = (xmlChar *)MODE_PATH;
    xmlNodePtr tmp;

    memset(&power, 0, sizeof(power));
    xmlDocPtr doc = xmlParseFile(PATH_SCENE_CONFIG);
    if (doc == NULL) {
        xmlCleanupParser();
        return 0;
    }

    xmlXPathObjectPtr result = get_node_set(doc, xpath);
    if (result == NULL) {
        ret = 0;
        goto out0;
    }

    xmlNodeSetPtr nodeset = result->nodesetval;
    ALOGD_IF(DEBUG_V, "nodeset->nodeNr = %d\n", nodeset->nodeNr);
    for (int i = 0; i < nodeset->nodeNr; i++) {
        xmlNodePtr cur = nodeset->nodeTab[i];

        if (parse_mode_node(cur) == 0) {
            ret = 0;
            break;
        }
    }

    xmlXPathFreeObject(result);
out0:
    xmlFreeDoc(doc);
    xmlCleanupParser();
    ALOGE_IF(ret == 0, "%s parse failed.", PATH_SCENE_CONFIG);

    return ret;
}

//###############################################
// For scene id define config file
static struct scene_id scene_ids[NUM_SCENE_MAX];
static int scene_id_count = 0;

/**
 * Translate power scene id to string name
 */
char *scene_id_to_string(int scene_id, int subtype)
{
    for (int i = 0; i < scene_id_count; i++) {
        if (scene_ids[i].id == scene_id && scene_ids[i].subtype == subtype) {
            return scene_ids[i].scene_name;
        }
    }

    return NULL;
}

int read_scene_id_define_file()
{
    FILE *fp = NULL;
    char buf[128] = {'\0'};
    char *p = NULL;
    int id = 0;
    int subtype = 0;
    struct scene_id *scene_id = NULL;

    memset(scene_ids, 0, sizeof(scene_ids));
    fp = fopen(PATH_SCENE_ID_DEFINE, "r");
    if (fp == NULL) {
        ALOGE("open failed(%s)", strerror(errno));
        return 0;
    }

    while (fgets(buf, sizeof(buf), fp)) {
        if (strlen(buf) < 5 || buf[0] == '#')
            continue;

        scene_id = scene_ids + scene_id_count;
        p = strtok(buf, " ");
        scene_id->id = strtol(p, NULL, 16);

        p = strtok(NULL, " ");
        scene_id->subtype = strtol(p, NULL, 16);

        p = strtok(NULL, " ");
        *(p + strlen(p) - 1) = '\0';
        strncpy(scene_id->scene_name, p, LEN_SCENE_NAME_MAX);

        ALOGD_IF(DEBUG, "0x%08x 0x%08x %s", scene_id->id, scene_id->subtype, scene_id->scene_name);

        scene_id_count++;
    }

    fclose(fp);
    return 1;
}

//###############################################
// Functions for the resource file
static int parse_file_node(xmlNodePtr cur)
{
    int ret = 1;
    xmlChar *path, *file, *name;
    xmlChar *no_has_def = NULL;
    xmlChar *comp_func, *set_func, *def_value;
    xmlChar *clear_func = NULL;
    struct file_node file_node;

    memset(&file_node, 0, sizeof(file_node));

    path = xmlGetProp(cur, (const xmlChar*)"path");
    file = xmlGetProp(cur, (const xmlChar*)"file");
    no_has_def = xmlGetProp(cur, (const xmlChar*)"no_has_def");
    cur = cur->xmlChildrenNode;

    ALOGD_IF(DEBUG_V, "<file path=\"%s\" file=\"%s\" no_has_def=\"%s\" />", path, file, no_has_def);

    while (cur) {
        if (strncmp("attr", (const char*)cur->name, 4) == 0) {
            def_value = NULL;
            name = xmlGetProp(cur, (const xmlChar*)"name");
            if (strncmp("comp_func",(const char*)name, 9) == 0) {
                comp_func = xmlGetProp(cur, (const xmlChar*)"value");
            } else if (strncmp("clear_func", (const char*)name, 10) == 0) {
                clear_func = xmlGetProp(cur, (const xmlChar*)"value");
            } else if (strncmp("set_func", (const char*)name, 8) == 0) {
                set_func = xmlGetProp(cur, (const xmlChar*)"value");
            } else if (strncmp("def_value", (const char*)name, 9) == 0) {
                def_value = xmlGetProp(cur, (const xmlChar*)"value");
            } else {
                ALOGE("!!!Don't support attr %s", name);
                if (name != NULL) xmlFree(name);
                ret = 0;
                goto out0;
            }
            xmlFree(name);
        }

        cur = cur->next;
    }

    if (DEBUG_V) {
        ALOGD("  <attr name=\"%s\" value=\"%s\" />", "comp_func", comp_func);
        ALOGD("  <attr name=\"%s\" value=\"%s\" />", "clear_func", clear_func);
        ALOGD("  <attr name=\"%s\" value=\"%s\" />", "set_func", set_func);
        if (def_value != NULL)
            ALOGD("  <attr name=\"%s\" value=\"%s\" />", "def_value", def_value);
        ALOGD("</file>");
    }

    if (path == NULL || file == NULL || comp_func == NULL || set_func == NULL) {
        ALOGE("!!!The resource file format is incorrect");
        ret = 0;
        goto out0;
    }

    file_node.path = (char *)path;
    file_node.file = (char *)file;
    file_node.no_has_def = (char *)no_has_def;
    file_node.comp = (char *)comp_func;
    file_node.clear = (char *)clear_func;
    file_node.set = (char *)set_func;
    file_node.def_value = (char *)def_value;

    init_file(&file_node);

out0:
    if (path != NULL) xmlFree(path);
    if (file != NULL) xmlFree(file);
    if (comp_func != NULL) xmlFree(comp_func);
    if (clear_func != NULL) xmlFree(clear_func);
    if (set_func != NULL) xmlFree(set_func);
    if (def_value != NULL) xmlFree(def_value);
    if (no_has_def != NULL) xmlFree(no_has_def);

    return ret;
}
/**
 * Parse inode node under subsys node in resource file
 */
static int parse_inode(xmlNodePtr cur, struct subsys *subsys)
{
    xmlChar *path, *file, *def_value, *no_has_def;
    struct subsys_inode *inode = NULL;

    if (CC_UNLIKELY(subsys == NULL || cur == NULL)) return 0;

    if (CC_UNLIKELY(subsys->inode_count >= NUM_FILE_MAX)) {
        ALOGE("!!!The inodes[] if full");
        return 0;
    }

    inode = &(subsys->inodes[subsys->inode_count++]);
    path = xmlGetProp(cur, (const xmlChar*) "path");
    file = xmlGetProp(cur, (const xmlChar*) "file");
    def_value = xmlGetProp(cur, (const xmlChar*) "def_value");
    no_has_def = xmlGetProp(cur, (const xmlChar*) "no_has_def");
    ALOGD_IF(DEBUG_V, "  <%s path=\"%s\" file=\"%s\" def_value=\"%s\" no_has_def=\"%s\" />"
            , cur->name, path, file, def_value, no_has_def);

    if (CC_UNLIKELY(path == NULL || file == NULL)) {
        if (path != NULL) xmlFree(path);
        if (file != NULL) xmlFree(file);
        return 0;
    }

    strncpy(inode->path, (const char *)path, LEN_PATH_MAX);
    if (inode->path[strlen(inode->path) - 1] == '/')
        inode->path[strlen(inode->path) - 1] = '\0';
    strncpy(inode->file, (const char *)file, LEN_FILE_MAX);

    xmlFree(path);
    xmlFree(file);

    if (def_value != NULL) {
        strncpy(inode->value.def_value, (const char *)def_value, LEN_VALUE_MAX);
        xmlFree(def_value);
    }
    if (no_has_def != NULL) {
        inode->no_has_def = atoi((const char *)no_has_def);
        xmlFree(no_has_def);
    } else {
        // By default, file has default value
        inode->no_has_def = 0;
    }

    return 1;
}
/**
 * Parse conf node under subsys node in resource file
 */
static int parse_conf(xmlNodePtr cur, struct subsys *subsys)
{
    int ret = 1;
    static int priority = 1;
    xmlChar *path, *file, *value, *name, *priority_str;
    struct config *config = NULL;
    struct set *set = NULL;

    if (subsys->config_count >= NUM_FILE_MAX) {
        ALOGE("!!!The inodes[] if full");
        return 0;
    }

    config = &(subsys->configs[subsys->config_count++]);
    name = xmlGetProp(cur, (const xmlChar*) "name");
    priority_str = xmlGetProp(cur, (const xmlChar*) "priority");
    if (priority_str != NULL) {
        priority = atoi((const char*)priority_str);
        xmlFree(priority_str);
    } else {
        priority++;
    }
    strncpy(config->name, (const char *)name, LEN_CONFIG_NAME_MAX);
    config->priority = priority;
    ALOGD_IF(DEBUG_V, "  <%s name=\"%s\" priority=\"%d\" />", cur->name, name, priority);
    xmlFree(name);

    cur = cur->xmlChildrenNode;
    while (cur) {
        if (xmlStrcmp(cur->name, (const xmlChar*) "set") == 0) {
            if (config->count >= NUM_FILE_MAX) {
                ALOGE("!!!The sets[] if full");
                return 0;
            }

            set = &(config->sets[config->count++]);
            path = xmlGetProp(cur, (const xmlChar*) "path");
            file = xmlGetProp(cur, (const xmlChar*) "file");
            value = xmlGetProp(cur, (const xmlChar*) "value");
            ALOGD_IF(DEBUG_V, "    <%s path=\"%s\" file=\"%s\" value=\"%s\" />", cur->name, path, file, value);
            if (CC_UNLIKELY(path == NULL || file == NULL || value == NULL)) {
                if (path != NULL) xmlFree(path);
                if (file != NULL) xmlFree(file);
                if (value != NULL) xmlFree(value);
                return 0;
            }

            strncpy(set->path, (const char *)path, LEN_PATH_MAX);
            if (set->path[strlen(set->path) - 1] == '/')
                set->path[strlen(set->path) - 1] = '\0';
            strncpy(set->file, (const char *)file, LEN_FILE_MAX);
            strncpy(set->value, (const char *)value, LEN_VALUE_MAX);

            xmlFree(path);
            xmlFree(file);
            xmlFree(value);
        }
        cur = cur->next;
    }
    if (DEBUG_V) ALOGD("  </conf>");

    return 1;
}

/**
 * Parse subsys node in resource file
 */
static int parse_subsys_node(xmlNodePtr cur)
{
    xmlChar *name = NULL;
    struct subsys *subsys = NULL;

    if (resources.subsys_count >= NUM_SUBSYS_MAX) {
        ALOGE("!!!subsystems[] if full");
        return 0;
    }
    subsys = &(resources.subsystems[resources.subsys_count++]);

    name = xmlGetProp(cur, (const xmlChar*) "name");
    strncpy(subsys->name, (const char *)name, LEN_SUBSYS_NAME_MAX);
    ALOGD_IF(DEBUG_V, "<%s name=\"%s\" />", cur->name, name);
    xmlFree(name);

    cur = cur->xmlChildrenNode;
    while (cur) {
        if (xmlStrcmp(cur->name, (const xmlChar*)"inode") == 0) {
            if (parse_inode(cur, subsys) == 0)
                return 0;
        } else if (xmlStrcmp(cur->name, (const xmlChar*)"conf") == 0) {
            if (parse_conf(cur, subsys) == 0)
                return 0;
        }

        cur = cur->next;
    }

    ALOGD_IF(DEBUG_V, "</subsys>");
    return 1;
}

int parse_resource_path(bool subsys)
{
    int ret = 1;
    xmlChar *xpath = NULL;
    xmlNodePtr tmp = NULL;

    if (subsys) {
        xpath = (xmlChar*)RESOURCE_SUBSYS_PATH;
    } else {
        xpath = (xmlChar*)RESOURCE_FILE_PATH;
    }

    xmlDocPtr doc = xmlParseFile(PATH_RESOURCE_FILE_INFO);
    if (doc == NULL) {
        xmlCleanupParser();
        return 0;
    }

    xmlXPathObjectPtr result = get_node_set(doc, xpath);
    if (result == NULL) {
        ret = 0;
        goto out0;
    }

    xmlNodeSetPtr nodeset = result->nodesetval;
    ALOGD_IF(DEBUG_V, "nodeset->nodeNr = %d\n", nodeset->nodeNr);
    for (int i = 0; i < nodeset->nodeNr; i++) {
        xmlNodePtr cur = nodeset->nodeTab[i];

        if (subsys) {
            ret = parse_subsys_node(cur);
        } else {
            ret = parse_file_node(cur);
        }
        if (ret == 0)
            break;
    }

    xmlXPathFreeObject(result);
out0:
    xmlFreeDoc(doc);
    xmlCleanupParser();

    ALOGE_IF(ret == 0, "%s parse failed.", PATH_RESOURCE_FILE_INFO);

    return ret;
}

/**
 * Parse the resource file
 */
int read_resource_config(void)
{
    if ((parse_resource_path(false) == 0) || (parse_resource_path(true) == 0))
        return 0;
    else
        return 1;
}
