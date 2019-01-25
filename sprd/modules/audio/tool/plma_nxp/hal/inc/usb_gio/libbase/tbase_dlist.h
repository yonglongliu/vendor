/*
 *Copyright 2014 NXP Semiconductors
 *
 *Licensed under the Apache License, Version 2.0 (the "License");
 *you may not use this file except in compliance with the License.
 *You may obtain a copy of the License at
 *            
 *http://www.apache.org/licenses/LICENSE-2.0
 *             
 *Unless required by applicable law or agreed to in writing, software
 *distributed under the License is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *See the License for the specific language governing permissions and
 *limitations under the License.
 */

/************************************************************************
 *  Module:       tbase_dlist.h
 *  Description:  double linked list macro implementation
 ************************************************************************/

#ifndef tbase_dlist_h_included
#define tbase_dlist_h_included

/*
  @doc

  @module DLIST <en-> Double Linked List | 
  The module DLIST provides an implementation of a double linked list.
  The DLIST structure and the related macros may be used for
  general purpose.

*/


/**********************************************************************

                 public part (interface)

**********************************************************************/


/*
 * Double linked list structure.                       
 * Can be used as either a list head, or as link words.
 */
struct tDLIST;
typedef struct tDLIST *PDLIST;


/*
 *  Double linked list manipulation routines.
 *  Implemented as macros but logically these are procedures.
 */

/*
 * void               
 * DlistInit(         
 *     PDLIST ListHead
 *     );             
 */
#define DlistInit(ListHead) \
    { (ListHead)->Flink = (ListHead)->Blink = (ListHead); }
/*
  @func void | DlistInit | 
    <f DlistInit> initializes a DLIST element. The link pointers
    points to the structure itself. This element represents
    an empty DLIST.
  @parm IN PDLIST<spc>| ListHead |
    Pointer to a structure of type DLIST.
  @comm Each list head has to be initialized by this function.
*/


/*
 * int
 * DlistEmpty(
 *     PDLIST ListHead
 *     );             
 */
#define DlistEmpty(ListHead) \
    ( (ListHead)->Flink == (ListHead) )
/*
  @func int | DlistEmpty |
    <f DlistEmpty> checks whether the list is empty.
  @parm IN PDLIST<spc>| ListHead |
    Pointer to the list head.
  @rdesc
    Returns 1 (TRUE) if the list is empty, 0 (FALSE) otherwise.
*/


/*
 * PDLIST
 * DlistGetNext(
 *     PDLIST Entry
 *     );              
 */
#define DlistGetNext(Entry) \
    ( (Entry)->Flink )
/*
  @func PDLIST<spc>| DlistGetNext |
    <f DlistGetNext> returns a pointer to the successor.
  @parm IN PDLIST<spc>| Entry |
    Pointer to a list entry.
  @rdesc
    Pointer to the successor of <p Entry>.
*/


/*
 * PDLIST
 * DlistGetPrev(
 *     PDLIST Entry
 *     );              
 */
#define DlistGetPrev(Entry) \
    ( (Entry)->Blink )
/*
  @func PDLIST<spc>| DlistGetPrev |
    <f DlistGetPrev> returns a pointer to the predecessor.
  @parm IN PDLIST<spc>| Entry |
    Pointer to a list entry.
  @rdesc
    Pointer to the predecessor of <p Entry>.
*/


/*
 * remove a single entry
 */

/*
 * void             
 * DlistRemoveEntry(
 *     PDLIST Entry 
 *     );           
 */
#define DlistRemoveEntry(Entry)                                      \
    {                                                                \
      PDLIST dlist_m_Entry = (Entry);                                \
      dlist_m_Entry->Blink->Flink = dlist_m_Entry->Flink;            \
      dlist_m_Entry->Flink->Blink = dlist_m_Entry->Blink;            \
      dlist_m_Entry->Flink = dlist_m_Entry->Blink = dlist_m_Entry;   \
    }
/*
  @func void | DlistRemoveEntry |
    <f DlistRemoveEntry> detaches one element from the list.
  @parm IN PDLIST<spc>| Entry |
    Pointer to the element to be detached.
  @comm Calling this function on an empty list results in
    undefined behaviour.
*/


/*
 * void
 * DlistRemoveHead(   
 *     PDLIST ListHead,
 *     PDLIST *Entry
 *     );             
 */
#define DlistRemoveHead(ListHead,Entry) \
    DlistRemoveEntry(*(Entry)=(ListHead)->Flink)
/*
  @func void | DlistRemoveHead |
    <f DlistRemoveHead> detaches the first element from the list.
  @parm IN PDLIST<spc>| ListHead |
    Pointer to the list head.
  @parm OUT PDLIST<spc>*| Entry |
    Address of a pointer to the detached element.
  @comm Calling this function on an empty list results in
    undefined behaviour.
*/


/*
 * void
 * DlistRemoveTail(   
 *     PDLIST ListHead,
 *     PDLIST *Entry
 *     );             
 */
#define DlistRemoveTail(ListHead,Entry) \
    DlistRemoveEntry(*(Entry)=(ListHead)->Blink)
/*
  @func void | DlistRemoveTail |
    <f DlistRemoveTail> detaches the last element from the list.
  @parm IN PDLIST<spc>| ListHead |
    Pointer to the list head.
  @parm OUT PDLIST<spc>*| Entry |
    Address of a pointer to the detached element.
  @comm Calling this function on an empty list results in
    undefined behaviour.
*/


/*
 * insert a single entry
 */

/*
 * void                
 * DlistInsertEntry(    
 *     PDLIST Entry,
 *     PDLIST NewEntry    
 *     );              
 *
 * inserts NewEntry after Entry
 */
#define DlistInsertEntry(Entry,NewEntry)                  \
    {                                                     \
      PDLIST dlist_m_Entry = (Entry);                     \
      PDLIST dlist_m_NewEntry = (NewEntry);               \
      dlist_m_NewEntry->Flink = dlist_m_Entry->Flink;     \
      dlist_m_NewEntry->Blink = dlist_m_Entry;            \
      dlist_m_Entry->Flink->Blink = dlist_m_NewEntry;     \
      dlist_m_Entry->Flink = dlist_m_NewEntry;            \
    }
/*
  @func void | DlistInsertEntry |
    <f DlistInsertEntry> inserts an element into a list.
  @parm IN PDLIST<spc>| Entry |
    Pointer to the element after which the new entry is to be inserted.
  @parm IN PDLIST<spc>| NewEntry |
    Pointer to the element to be inserted.
  @comm <p NewEntry> is inserted after <p Entry>, i. e. <p NewEntry>
    becomes the successor of <p Entry>.
*/


/*
 * void                
 * DlistInsertHead(    
 *     PDLIST ListHead,
 *     PDLIST Entry    
 *     );              
 */
#define DlistInsertHead(ListHead,Entry)   \
    DlistInsertEntry(ListHead,Entry)
/*
  @func void | DlistInsertHead |
    <f DlistInsertHead> inserts an element at the beginning of a list.
  @parm IN PDLIST<spc>| ListHead |
    Pointer to the list head.
  @parm IN PDLIST<spc>| Entry |
    Pointer to the element to be inserted.
  @comm <p Entry> becomes the first list entry.
*/


/*
 * void                
 * DlistInsertTail(    
 *     PDLIST ListHead,
 *     PDLIST Entry    
 *     );              
 */
#define DlistInsertTail(ListHead,Entry)   \
    DlistInsertEntry((ListHead)->Blink,Entry)
/*
  @func void | DlistInsertTail |
    <f DlistInsertTail> inserts an element at the end of a list.
  @parm IN PDLIST<spc>| ListHead |
    Pointer to the list head.
  @parm IN PDLIST<spc>| Entry |
    Pointer to the element to be inserted.
  @comm <p Entry> becomes the last list entry.
*/


/*
 * append another list
 */

/*
 * void                
 * DlistAppend(    
 *     PDLIST ListHead,
 *     PDLIST List    
 *     );              
 *
 * appends List to tail of ListHead
 */
#define DlistAppend(ListHead,List)                \
    {                                             \
      PDLIST dlist_m_List = (List);               \
      PDLIST dlist_m_Tail = (ListHead)->Blink;    \
      dlist_m_Tail->Flink = dlist_m_List;         \
      dlist_m_List->Blink->Flink = (ListHead);    \
      (ListHead)->Blink = dlist_m_List->Blink;    \
      dlist_m_List->Blink = dlist_m_Tail;         \
    }
/*
  @func void | DlistAppend |
    <f DlistAppend> concatenates two lists.
  @parm IN PDLIST<spc>| ListHead |
    Pointer to the list head of the first list.
  @parm IN PDLIST<spc>| List |
    Pointer to the list head of the second list.
  @comm The first element of <p List> becomes the successor 
    of the last element of <p ListHead>.
*/


/**********************************************************************

                 private part (implementation)

**********************************************************************/

/*
 * Double linked list structure.                       
 * Can be used as either a list head, or as link words.
 */
typedef struct tDLIST {

   struct tDLIST* Flink;
   struct tDLIST* Blink;

} DLIST;

/*
  @struct DLIST |
    The DLIST structure is the link element of the double linked list.
    It is used as either a list head, or as link entry.
  @field  struct tDLIST * | Flink |
    Pointer to the successor (forward link).
  @field  struct tDLIST * | Blink |
    Pointer to the predecessor (backward link).
  @comm By means of such elements any structures may be handled
    as a double linked list. The DLIST structure is to be inserted
    into the structure which is to be handled. A pointer to the 
    original structure can be obtained by means of the macro
    <f TB_BASE_POINTER_FROM_MEMBER>.
*/


#endif  /* tbase_dlist_h_included */

/*************************** EOF **************************************/
