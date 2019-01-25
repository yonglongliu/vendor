#ifndef  __XMLPARSE_ZSJ_
#define  __XMLPARSE_ZSJ_

#include "tinyxml.h"
#include "tinystr.h"
#include <string.h>

//生成xml文件
bool CreateXmlFile (char *filename, char *contentbuff);

//加载xml文件
TiXmlDocument *LoadXml (char *filename, FILE *fp = NULL);

//将xml文件加载到内存
TiXmlDocument *LoadXmlString (char *xmlstr);

//保存xml文档
bool SaveXml (TiXmlDocument *doc, FILE *fp = NULL);

//卸载xml，释放内存
void FreeXml (TiXmlDocument *doc);

//遍历DOM节点
bool VisitDocument (TiXmlDocument *doc, FILE *fp);

//TiXmlElement *QueryDocument (TiXmlDocument *doc, FILE *fp, char *queryName) const;

bool VisitBuffer (TiXmlDocument *doc, char *pbuf, long len);
//打印出各个节点
bool DisplayXmlNode (TiXmlElement *node, int i);

//根据输入的条件，查询该节点的值
bool ParseXmlNode (TiXmlElement *node, char *nodename , char *attrname ,const char *index, char *attrvalue);

//根据节点的name，返回其text值，成功时，value为其值，否则value = NULL
bool GetElementValue (TiXmlDocument *doc, char *name, char *value);

//取得节点的某个属性值，如没有此属性，则返回NULL,返回值由attrValue带出
void GetElementAttrib (TiXmlElement *pEle, char *attrName, char *attrValue);

//取得某个节点第n个子节点的值，如存在则返回值，否则返回NULL
TiXmlElement *GetElementNode (TiXmlElement *pEle, char *nodename, int index);

//获取给定节点的某个子节点的值
bool getElementValue(TiXmlElement *parElement,char *elementName,char *elementValue,int valueLength);

//给定一个节点，循环取得其text值，如没有值则返回false
bool GetElementValueByEle(TiXmlElement *pSrcEle, char *value);

//拷贝xml文件到其他目录
bool CopyXml (char *destpath, char *filename);

//取DOM文档模型的root节点
TiXmlNode *GetRootNode (TiXmlDocument * doc);

//根据节点的名称，返回节点的值，先查询子节点，再查询兄弟节点，若不存在，则返回NULL
TiXmlElement * SearchNode(TiXmlElement *element,char * nodename);

//根据节点的名称，返回节点的值，仅从给定节点的子节点查询，若不存在，则返回NULL
TiXmlElement * search(TiXmlElement *element,char * nodename);

//设置text属性的值
bool SetTextAttribute (TiXmlDocument *doc, char *name, char *value);
bool SetElementText (TiXmlElement *pElement, char *textvalue);

//根据条件，设置节点某个属性的值，如不存在则添加某个属性值
bool SetElementAttrib (TiXmlElement *pEle, char *attrName, char *attrValue);

//插入子节点
TiXmlElement *InsertElementNode (TiXmlElement *pParentElement, char *nodename);

//在给定节点前添加新节点
TiXmlElement *InsertBeforeElementNode (TiXmlElement *pEle, char *nodename);

//在给定节点后插入新节点
TiXmlElement *InsertAfterElementNode (TiXmlElement *pEle, char *nodename);

//删除节点
bool DelElementNode (TiXmlElement *pEle);

//删除节点的属性值
bool DelElementAttribute (TiXmlElement *pElement, char *attrname);

#endif
