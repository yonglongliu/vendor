/*
*
*"myxmltest.cpp"
* author   :     zhangshunjun
* createime:     2007-08-07
* Copyright:     vcom
* E_mail   :     zhangshunjun@zzvcom.com
* 此代码仅用于演示封装操作xml文件的函数
*
*/
#include "xmlparse.h"

#define  SUCCESS   0
#define  FAIL      -1

#define BUFSIZE    256

void mprint (char *str)
{
    printf ("\e[1;32m[%s]\e[0m\n", str);

    return;
}

int main (int argc, char **argv)
{
    TiXmlDocument *m_pDoc = NULL;  //文档对象指针
    TiXmlElement *m_pRootElement = NULL;   //元素节点指针
    TiXmlElement *m_element = NULL;
    TiXmlElement *pInsertNode = NULL;
    TiXmlNode *node = NULL;        //节点指针

    char buff[1024] = {};
    char *pbNodename = "bold";
    char *pNodename = "test";
    char *pAttribtename = "distance";
    char *xmlfilename = "testxml.xml";
    char *xmlbuff =
        "<?xml version=\"1.0\"  standalone='no' >\n"
        "<!-- Our to do list data -->"
        "<ToDo>\n"
        "<!-- Do I need a secure PDA? -->\n"
        "<Item priority=\"1\" distance='close'> Go to the <bold>Toy store!</bold></Item>"
        "<Item priority=\"2\" distance='none'> Do bills   </Item>"
        "<Item priority=\"2\" distance='far &amp; back'> Look for Evil Dinosaurs! </Item>"
        "</ToDo>";

    char valuebuf[BUFSIZE];

    //create xml file
    mprint ("begin to create xml file......");
    if (CreateXmlFile (xmlfilename, xmlbuff) != true)
    {
        printf ("create xml file failed\n");
        return FAIL;
    }
    else
    {
        mprint ("create xml file successfully");
    }

    //从磁盘读取xml文件
    mprint ("loading xml from disk......");
    m_pDoc = LoadXml (xmlfilename);
    if (m_pDoc == NULL)
    {
        printf ("load xml file failed\n");
        return FAIL;
    }
    mprint ("loading xml successfully");

    //遍历xml文件
    mprint ("visit xml file......");
    {
        VisitDocument (m_pDoc, stdout);
    }

    //取得根节点
    node = GetRootNode (m_pDoc);
    if (node  == NULL)
    {
        printf ("get root node failed\n");
        FreeXml (m_pDoc);

        return FAIL;
    }
    m_pRootElement = node->ToElement ();

    //添加节点
    mprint ("insert one node after bold node......");
    //查找待插接点的后一个节点
    m_element = SearchNode (m_pRootElement, pbNodename);
    if (m_element == NULL)
    {
        printf ("not exist node %s\n", pbNodename);

        //设置插在第一个节点的后面
        m_element = GetElementNode (m_pRootElement, "", 1);
    }

    pInsertNode = InsertBeforeElementNode (m_element, pNodename);
    InsertAfterElementNode (m_element, pNodename);

    VisitDocument (m_pDoc, stdout);
    printf ("\n");
    //设置新插节点的属性值
    mprint ("set attribute value......");
    SetElementAttrib (pInsertNode, pAttribtename, "test");

    //设置文本属性
    mprint ("set text value......");
    if (SetTextAttribute (m_pDoc, pNodename, "text") == false)
    {
        printf ("set text failed\n");
    }

    if (SetElementAttrib (pInsertNode, "mytest", "test") == false)
    {
        printf ("set text failed\n");
    }

    VisitDocument (m_pDoc, stdout);
    printf ("\n");


    DisplayXmlNode (m_pRootElement, 1);
    //查询节点
    printf ("select node %s......\n", pNodename);
    printf ("%s\n", m_element->Value());

    TiXmlElement *pTmpNode = GetElementNode (m_element->Parent()->ToElement(), pNodename, 1);
    if (pTmpNode != NULL)
    {
        printf ("%s:\n", pTmpNode->Value ());
    }
    else
    {
        printf ("not find %s node\n", pNodename);
    }

    memset (valuebuf, 0, sizeof (valuebuf));
    GetElementAttrib (pInsertNode, pAttribtename, valuebuf);
    printf ("\tattribute %s:%s\n", pAttribtename, valuebuf);

    memset (valuebuf, 0, sizeof (valuebuf));
    GetElementValueByEle (pInsertNode, valuebuf);
    printf ("\ttext value:%s\n", valuebuf);

    //删除属性
    DelElementAttribute (pInsertNode, "mytest");
    VisitDocument (m_pDoc, stdout);
    printf ("\n");

    //删除节点
    printf ("delete node %s......\n", pNodename);
    DelElementNode (pInsertNode);

    VisitDocument (m_pDoc, stdout);
    printf ("\n");

    SaveXml (m_pDoc);

    FreeXml (m_pDoc);

    return SUCCESS;
}