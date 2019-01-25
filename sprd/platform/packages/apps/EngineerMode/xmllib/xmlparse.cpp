/*
 *  file: "xmlparse.cpp"
 *  author:zsj
 *  time: 2007-04-12
 */
#include "xmlparse.h"
#include "ATProcesser.h"
#include <iostream>
#include <sstream>
#include <fstream>
//#define BUILD_ENG
#include <string>
#ifdef BUILD_ENG
//#include "eng_appclient_lib.h"
//#include "AtChannel.h"
#include "sprd_atci.h"
#endif
#include "HTTPRequest.h"
#include "./xmllib/xmlparse.h"
#ifdef ANDROID
#include <cutils/log.h>
#include <android/log.h>
#define MYLOG(args...)  __android_log_print(ANDROID_LOG_INFO,  "HAOYOUGEN", ## args)
#endif
//生成xml文件
/*
*
*filename:文件名
*contentbuff:文件的内容
*返回值：若创建成功，返回true;否则，返回false
*
*/
bool CreateXmlFile (char *filename, char *contentbuff)
{
    printf ("############################## CreateXmlFile \n");
    if ((*filename == 0) || (*contentbuff == 0))
    {
        MYLOG ("filename or contentbuff is null\n");
        return false;
    }

    TiXmlDocument *doc = new TiXmlDocument(filename);

    if (doc == NULL)
    {
        MYLOG ("mem overflow!\n");
        return false;
    }

    doc->Parse(contentbuff);

    if ( doc->Error() )
    {
        MYLOG( "Error in %s: %s\n", doc->Value(), doc->ErrorDesc() );
        return false;
    }

    doc->SaveFile();

    delete doc;
    doc = NULL;

    return true;
}

//加载xml文件
TiXmlDocument *LoadXml (char *filename, FILE *fp)
{
    if (filename == NULL)
    {
        return NULL;
    }

    TiXmlDocument *doc = new TiXmlDocument(filename);
    if (fp == NULL)
    {
        if (doc->LoadFile () != true)
        {
            doc = NULL;
        }
    }
    else
    {
        if (doc->LoadFile (fp) != true)
        {
            doc = NULL;
        }
    }

    return doc;
}

TiXmlDocument *LoadXmlString (char *xmlstr)
{
    if ((xmlstr == NULL) || (*xmlstr == 0))
    {
        return NULL;
    }

    TiXmlDocument *doc = new TiXmlDocument();

    doc->Parse (xmlstr);

    return doc;
}
//保存xml文档
bool SaveXml (TiXmlDocument *doc, FILE *fp/*=NULL*/)
{
    bool ret = true;

    if (fp != NULL)
    {
        if (doc->SaveFile (fp) != true)
        {
            ret = false;
        }
    }
    else
    {
        if (doc->SaveFile () != true)
        {
            ret = false;
        }
    }

    return ret;
}
//拷贝xml文件到其他目录
bool CopyXml (char *destpath, char *filename)
{
    assert (destpath&&filename);
    FILE *fp = NULL;
    char tmpstr[50000];
    char file[50];

    memset (tmpstr, 0, sizeof (tmpstr));
    memset (file, 0, sizeof (file));

    sprintf (file, "%s/%s", destpath, filename);

    if ((fp = fopen (filename, "r")) == NULL)
    {
        printf ("Can not open %s\n", file);
        return false;
    }
    char c;
    int i = 0;
    c = fgetc (fp);

    while (c != EOF)
    {
        tmpstr[i++] = c;
        c = fgetc (fp);
    }

    tmpstr [i] = '\0';

    TiXmlDocument doc (file);
    doc.Parse (tmpstr);
    doc.SaveFile ();

    return true;
}


//卸载xml文件，释放内存，与LoadXml配合使用
void FreeXml (TiXmlDocument *doc)
{
    if (doc != NULL)
        delete doc;
}

//取DOM文档模型的root节点
TiXmlNode *GetRootNode (TiXmlDocument * doc)
{
    TiXmlNode *node = NULL;
    if (doc == NULL)
    {
        return NULL;
    }
    node = doc -> RootElement ();

    return node;
}
//显示模型各个节点
bool DisplayXmlNode (TiXmlElement *node, int i)
{
    //printf("The node:%s",node->Value());
    TiXmlElement *tmpnode=0;
    TiXmlText *text=0;

    printf("The number %d node:%s\n",i,node->Value());

    //如果有属性的话打印出其节点的属性
    if(node->FirstAttribute())
    {
        TiXmlAttribute *att=0;
        att=node->FirstAttribute();
        while(att)
        {
            printf("  Attribute:%s=%s\n",att->Name(),att->Value());
            att=att->Next();
        }
    }
    //如果是文本节点的话打印出文本节点的内容
    if (node->FirstChild())
    {
        if(text=node->FirstChild()->ToText())
        {
            printf("  Text:%s\n",text->Value());
        //  return 1;
        }
    }

    if(node->NoChildren())//没有孩子节点返回
        return 1;
    else//若有孩子节点，则循环查找
    {
        int j = 1;
        tmpnode=node->FirstChildElement();
        while(tmpnode)
        {
           DisplayXmlNode(tmpnode,j);
           tmpnode=tmpnode->NextSiblingElement();
           j++;
        }


        return 1;
    }
}
//根据输入的条件，查询该节点的值
/*
 *  node 节点查询的开始节点
 *  nodename 要查询节点的标识
 *  attrname 要节点的属性标识
 *  index    查询的属性值索引
 *  attrvalue
 */
bool ParseXmlNode (TiXmlElement *node, char *nodename , char *attrname , const char *index, char *attrvalue)
{
    TiXmlElement *pEle = NULL;
    TiXmlElement *pChild = NULL;
    TiXmlAttribute *m_pAttr = NULL;
    bool flg = false;

    if (node == NULL)
    {
        attrvalue = NULL;
        flg = true;
        return true;
    }

    //查找此节点的是否信息
    //printf ("node->Value: %s\nnodename = %s\nattrname = %s\nindex = %s\n",node->Value(),nodename,attrname,index);
    if (!strcmp (node->Value(), nodename))
    {
        m_pAttr = node -> FirstAttribute();

        while(m_pAttr)
        {
            if (!strcmp (m_pAttr -> Name(), attrname) && !strcmp (m_pAttr->Value(), index))
            {
                if (GetElementValueByEle (node, attrvalue) == true)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }

            m_pAttr=m_pAttr->Next();
        }
    }
    //没有孩子节点
    if (node -> NoChildren ())
    {
        for (pEle = node ->NextSiblingElement (nodename); pEle; pEle = pEle->NextSiblingElement(nodename))
        {
            if (ParseXmlNode (pEle, nodename, attrname, index, attrvalue) == true)
            {
                flg = true;
                return true;
            }
        }
    }
    else
    {
        for (pChild = node ->FirstChildElement (); pChild; pChild = pChild->NextSiblingElement())
        {
            if (ParseXmlNode (pChild, nodename, attrname, index, attrvalue) == true)
            {
                flg = true;
                return true;
            }
        }
    }
    if (!flg)
        return flg;
    return false;
}
//返回element value，成功时，value为其值，否则value = NULL
/*
 *  doc:xml文件
 *  name:节点的名称
 *  value: 节点的值，若能查到name节点，则返回value值，否则value为NULL
 */
bool GetElementValue (TiXmlDocument *doc, char *name, char *value)
{
    TiXmlElement *pEle;
    TiXmlText *pText = NULL;

    if (doc == NULL)
    {
        *value = 0;
        return false;
    }

    //获取根结点
    pEle = doc ->RootElement() -> ToElement();
    if (pEle)
    {
        //循环查找符合条件的节点
        pEle = SearchNode (pEle, name);
        if (pEle == NULL)
        {
            *value = 0;

            return false;
        }

        //查找符合条件的节点的text值
        if (GetElementValueByEle (pEle, value) == true)
        {
            return true;
        }
    }

    *value = 0;
    return false;
}

//给定一个节点，循环取得其值(仅能取得text的值)，如没有值则返回false
bool GetElementValueByEle(TiXmlElement *pSrcEle, char *value)
{
    if (pSrcEle == NULL)
    {
        *value = 0;
        return false;
    }

    TiXmlElement *pEle = pSrcEle;
    TiXmlText *pText = NULL;
    char *ptextBuf = NULL;

    while (pEle)
    {
        if (pEle->FirstChild())
        {
            if (pText = pEle ->FirstChild() ->ToText ())
            {
                strcpy (value, pText ->Value());

                return true;
            }
            else
            {
                pEle = pEle->FirstChildElement();
            }
        }
        else
        {
            pEle = NULL;
            break;
        }
    }

    *value = 0;

    return false;
}

////如果给的节点有子节点的话，可以在子节点中查找，成功的话返回节点信息，及其母节点信息。
//修改类库的一个bug，当节点没有字节点时，直接返回NULL
TiXmlElement * search(TiXmlElement *element,char * nodename)
{
    TiXmlElement * tmpelement=0;
    TiXmlElement * m_element = 0;
    tmpelement=element;

    if (element == NULL)
    {
        return NULL;
    }

    if (element->Value() == NULL)
    {
        return NULL;
    }

    if(strcmp(tmpelement->Value(),nodename)==0)
    {
#ifdef bDebug
        MYLOG("#################### %s's parent node is: %s\n",nodename,tmpelement->Parent()->Value());
#endif
        return tmpelement;
    }
    else
    {
        if(tmpelement->NoChildren() == false)
        {
            tmpelement=tmpelement->FirstChildElement();
            while (tmpelement != NULL)
            {
                MYLOG ("########## tmpname = %s\n", tmpelement->Value());
                if((m_element = search(tmpelement,nodename)) == NULL)
                {
                    tmpelement=tmpelement->NextSiblingElement();
                }
                else
                {
                    return m_element;
                }
            }

            return NULL;
        }
        else
        {//没有孩子则返回NULL
            return NULL;
        }
    }

}

////查找xml文件中从element开始的符合查找条件的节点,
////成功时返回符合条件的第一个节点及其打印出其母节点信息,否则返回失败信息.
TiXmlElement * SearchNode(TiXmlElement *element,char * nodename)
{
    TiXmlElement *tmpelement=0;
    TiXmlElement *tmp=0;
    tmpelement=element;

    while (tmpelement)
    {
        tmp = NULL;
        tmp=search(tmpelement,nodename);

        if(tmp != NULL)
        {
        //  printf ("tmpvalue = %s\n", tmp->Value());
            break;
        }
        else
            tmpelement=tmpelement->NextSiblingElement();

    }
    if(tmp)
    {
#ifdef bDebug
        printf("find %s successful!\n", tmp->Value());
#endif

        return tmp;
    }
    else
    {
#ifdef bDebug
        printf("Can't find node %s\n",nodename);
#endif
        return NULL;
    }

}
//遍历DOM节点
bool VisitDocument (TiXmlDocument *doc, FILE *fp)
{
    TiXmlPrinter printer;
    doc->Accept( &printer );
    MYLOG("~~~~~~~~~~~ VisitDocument %s is not permitted ", printer.CStr());
    return true;
}

//遍历DOM节点
/*TiXmlElement *QueryDocument (TiXmlDocument *doc, FILE *fp, char *queryname)
{
    TiXmlElement *tx = NULL;
    TiXmlPrinter printer;
    MYLOG("~~~~~~~~~~~ lfok QueryDocument begin ");
    tx = doc->QueryElement( &printer, queryname);
    MYLOG("~~~~~~~~~~~ lfok QueryDocument end %s is not permitted ", printer.CStr());
    return tx;
}*/

bool VisitBuffer (TiXmlDocument *doc, char *pbuf, long len)
{
    TiXmlPrinter printer;
    doc->Accept( &printer );
    snprintf( pbuf, len, "%s", printer.CStr() );

    return true;
}
//设置节点的text属性值
bool SetElementText (TiXmlElement *pElement, char *textvalue)
{
    if (pElement == NULL)
    {
        MYLOG ("pElement is NULL\n");
        return false;
    }

    TiXmlText *pText = NULL;

    if (pElement->FirstChild())
    {
        if (pText = pElement ->FirstChild() ->ToText ())
        {
            pText ->SetValue (textvalue);
            return true;
        }
    }

    pText = new TiXmlText (textvalue);
    pElement->InsertEndChild (*pText);

    delete pText;
    pText = NULL;

    return true;

}
//设置属性的值,仅指text的值
bool SetTextAttribute (TiXmlDocument *doc, char *name, char *value)
{
    TiXmlElement *pEle, *pRootElement;
    TiXmlText *pText = NULL;

    pRootElement = doc ->RootElement() -> ToElement();
    if (pRootElement)
    {
        pEle =SearchNode (pRootElement, name);

        if (pEle)
        {
            if (SetElementText(pEle, value) == true)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            printf ("not find %s node\n", name);
            return false;
        }
    }

    return false;
}

//根据条件，设置节点某个属性的值，如不存在则添加某个属性值
bool SetElementAttrib (TiXmlElement *pEle, char *attrName, char *attrValue)
{
    if ((pEle == NULL) || (attrName == NULL) || (attrValue == NULL))
    {
        printf ("Set attrib failed!\n");
        return false;
    }

    pEle->SetAttribute (attrName, attrValue);

    return true;
}

//取得某个节点第n个子节点的值，如存在则返回值，否则返回NULL
/*
 *  pEle: 父节点名
 *  nodename: 子节点的名字
 *  index: 标识第几个子节点
 */
TiXmlElement *GetElementNode (TiXmlElement *pEle, char *nodename, int index)
{
    if (pEle == NULL)
    {
        printf ("pEle is NULL\n");
        return NULL;
    }

    TiXmlElement *pReElement = NULL;

    if (pEle->NoChildren())
    {
        printf ("No Children\n");
        return pReElement;
    }

    if ((*nodename == '\0') || (nodename == NULL))
        pReElement = pEle->FirstChildElement();
    else
        pReElement = pEle->FirstChildElement(nodename);

    while ((--index) && pReElement)
    {
        if ((*nodename == '\0') || (nodename == NULL))
            pReElement = pReElement->NextSiblingElement();
        else
            pReElement = pReElement->NextSiblingElement(nodename);
    }

    return pReElement;
}

//取得节点的某个属性值，如没有此属性，则返回NULL,返回值由attrValue带出
void GetElementAttrib (TiXmlElement *pEle, char *attrName, char *attrValue)
{
    if ((pEle == NULL) || (attrName == NULL))
    {
        printf ("Get attrib failed!\n");
        return;
    }

    TiXmlAttribute *m_pAttr = NULL;

    m_pAttr = pEle->FirstAttribute();

    while (m_pAttr)
    {
        if (!strcmp (m_pAttr->Name(), attrName))
        {
            strcpy (attrValue, m_pAttr->Value());
            return;
        }
        m_pAttr = m_pAttr->Next();
    }

    attrValue = NULL;
    return;
}

//获取给定节点的某个子节点的值
//输入：
//    parElement 父节点
//    elementName 子节点的名称
//    valueLength  子节点值的大小
//输出：
//    elementValue  子节点的值
bool getElementValue(TiXmlElement *parElement,char *elementName,char *elementValue,int valueLength)
{
    TiXmlElement *Element=NULL;
    Element=GetElementNode(parElement,elementName,1);
    if(Element==NULL)
    {
        printf("%s node is null\n", elementName);
        return false;
    }

    memset(elementValue,0,valueLength);
    if(GetElementValueByEle(Element,elementValue))
    {
        printf("%s value=%s\n", elementName,elementValue);
        return true;
    }
    else
    {
        printf("%s value is null\n", elementName);
        return false;
    }
}

//在给定节点前添加节点element
TiXmlElement * InsertBeforeElementNode (TiXmlElement *pEle, char *nodename)
{
    if ((pEle == NULL) || (*nodename == '\0') || (nodename == NULL))
    {
        printf ("parent node not exists!\n");
        return NULL;
    }

    TiXmlNode *parentNode = NULL;
    TiXmlNode *tmpnode = NULL;
    TiXmlElement tmpElement(nodename);

    parentNode = pEle->Parent();
    tmpnode = parentNode->InsertBeforeChild (pEle, tmpElement);

    if (tmpnode)
        return tmpnode->ToElement();
    else
        return NULL;
}

//在给定节点后插入新节点
TiXmlElement *InsertAfterElementNode (TiXmlElement *pEle, char *nodename)
{
    if ((pEle == NULL) || (*nodename == '\0') || (nodename == NULL))
    {
        printf ("parent node not exists!\n");
        return NULL;
    }

    TiXmlNode *parentNode = NULL;
    TiXmlNode *tmpnode = NULL;
    TiXmlElement tmpElement(nodename);

    parentNode = pEle->Parent();
    tmpnode = parentNode->InsertAfterChild (pEle, tmpElement);

    if (tmpnode)
        return tmpnode->ToElement();    
    else
        return NULL;
}


//插入子节点
TiXmlElement *InsertElementNode (TiXmlElement *pParentElement, char *nodename)
{
    if ((pParentElement == NULL) || (*nodename == '\0') || (nodename == NULL))
    {
        printf ("parent node not exists!\n");
        return NULL;
    }

    TiXmlNode *tmpnode = NULL;
    TiXmlElement tmpElement(nodename);

    tmpnode = pParentElement->InsertEndChild (tmpElement);

    if (tmpnode)
        return tmpnode->ToElement();
    else
        return NULL;
}

//删除节点
bool DelElementNode (TiXmlElement *pEle)
{

    if (pEle == NULL)
        return false;

    TiXmlNode *parentNode = NULL;

    parentNode = pEle->Parent();

    if (parentNode)
    {
        parentNode->RemoveChild(pEle);
        return true;
    }

    return false;
}

//删除节点的属性值
bool DelElementAttribute (TiXmlElement *pElement, char *attrname)
{
    if (pElement == NULL)
    {
        return false;
    }

    pElement->RemoveAttribute (attrname);

    return true;
}
