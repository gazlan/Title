/* ******************************************************************** **
** @@ Title Src File
** @  Copyrt :
** @  Author :
** @  Modify :
** @  Update :
** @  Notes  : Create/Replace HTML <title> tag with first occurrent of <h1>content</h1>
** ******************************************************************** */

/* ******************************************************************** **
**                uses pre-compiled headers
** ******************************************************************** */

#include <stdafx.h>

#include "..\shared\mmf.h"
#include "..\shared\text.h"
#include "..\shared\vector.h"
#include "..\shared\vector_sorted.h"
#include "..\shared\file.h"
#include "..\shared\file_walker.h"
#include "..\shared\search_bmh.h"

/* ******************************************************************** **
** @@                   internal defines
** ******************************************************************** */

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef NDEBUG
#pragma optimize("gsy",on)
#pragma comment(linker,"/FILEALIGN:512 /MERGE:.rdata=.text /MERGE:.data=.text /SECTION:.text,EWR /IGNORE:4078")
#endif

#define BEGIN_OF_TITLE        "<title>"
#define END_OF_TITLE          "</title>"

#define END_OF_HEAD           "</head>"
#define EOL_MARKER            "\r\n"

#define BEGIN_OF_HEADER1      "<h1>"
#define END_OF_HEADER1        "</h1>"

struct TANDEM
{
   int   _iStart;
   int   _iFinish;
};

/* ******************************************************************** **
** @@                   internal prototypes
** ******************************************************************** */

/* ******************************************************************** **
** @@                   external global variables
** ******************************************************************** */

extern DWORD   dwKeepError = 0;

/* ******************************************************************** **
** @@                   static global variables
** ******************************************************************** */
                  
static MMF        MF;
                  
static TANDEM     _Title;
static TANDEM     _Head;
static TANDEM     _Header1;

/* ******************************************************************** **
** @@                   real code
** ******************************************************************** */

/* ******************************************************************** **
** @@ ForEach()
** @  Copyrt :
** @  Author :
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

void ForEach(const char* const pszFileName)
{
   memset(&_Title,  0,sizeof(TANDEM));
   memset(&_Head,   0,sizeof(TANDEM));
   memset(&_Header1,0,sizeof(TANDEM));

   char     pszBackName[_MAX_PATH];
   char     pszDrive   [_MAX_DRIVE];
   char     pszDir     [_MAX_DIR];
   char     pszFName   [_MAX_FNAME];
   char     pszExt     [_MAX_EXT];

   _splitpath(pszFileName,pszDrive,pszDir,pszFName,pszExt);
   _makepath( pszBackName,pszDrive,pszDir,pszFName,"BAK");

   CopyFile(pszFileName,pszBackName,FALSE);

   HANDLE   hFile = CreateFile(pszFileName,CREATE_ALWAYS,0);

   VERIFY(hFile != INVALID_HANDLE_VALUE);

   if (hFile == INVALID_HANDLE_VALUE)
   {
      // Error !
      CloseHandle(hFile);
      hFile = INVALID_HANDLE_VALUE;
      return;
   }

   if (!MF.OpenReadOnly(pszBackName))
   {
      // Error !
      CloseHandle(hFile);
      hFile = INVALID_HANDLE_VALUE;
      return;
   }

   BYTE*    pText  = MF.Buffer();
   DWORD    dwSize = MF.Size();

   bool     bTitlePresent = false;
   bool     bHdrPresent   = false;

   // Title
   _Title._iStart = BMH_ISearch(pText,dwSize,(BYTE*)BEGIN_OF_TITLE,sizeof(BEGIN_OF_TITLE) - 1);

   if (_Title._iStart == -1)
   {
      printf(">> %s:\n[*] Warning: No <title> HTML tag found in the file.\n\n",pszFileName);
   }

   if (_Title._iStart != -1)
   {
      _Title._iFinish = BMH_ISearch(pText,dwSize,(BYTE*)END_OF_TITLE,sizeof(END_OF_TITLE) - 1);

      bTitlePresent = _Title._iFinish > _Title._iStart  ?  true  :  false;

      if (_Title._iFinish < _Title._iStart)
      {
         printf(">> %s:\n[!] Error: Incorrect HTML file.\nOccurrent </title> tag before <title> tag.\n",pszFileName);
         printf("</title> tag position %08X\n",_Title._iFinish);
         printf("<title> tag position  %08X\n\n",_Title._iStart);
      }
   }

   // End of Head
   _Head._iFinish = BMH_ISearch(pText,dwSize,(BYTE*)END_OF_HEAD,sizeof(END_OF_HEAD) - 1);

   if (_Head._iFinish == -1)
   {
      // Error !
      CloseHandle(hFile);
      hFile = INVALID_HANDLE_VALUE;
      MF.Close();
      printf(">> %s:\n[!] Error: Incorrect HTML file.\nNo </head> tag found in the file. Aborted.\n\n",pszFileName);
      return;
   }

   // Header1
   _Header1._iStart = BMH_ISearch(pText,dwSize,(BYTE*)BEGIN_OF_HEADER1,sizeof(BEGIN_OF_HEADER1) - 1);

   if (_Header1._iStart == -1)
   {
      printf(">> %s:\n[*] Warning: No <H1> HTML tag found in the file.\n\n",pszFileName);
   }

   if (_Header1._iStart != -1)
   {
      _Header1._iFinish = BMH_ISearch(pText,dwSize,(BYTE*)END_OF_HEADER1,sizeof(END_OF_HEADER1) - 1);

      bHdrPresent = _Header1._iFinish > _Header1._iStart  ?  true  :  false;
     
      if (_Header1._iFinish < _Header1._iStart)
      {
         printf(">> %s:\n[!] Error: Incorrect HTML file.\nOccurrent </H1> tag before <H1> tag.\n",pszFileName);
         printf("</H1> tag position %08X\n",_Header1._iFinish);
         printf("<H1> tag position  %08X\n\n",_Header1._iStart);
      }
   }

   char     pszHeader[MAX_PATH + 1];

   bool     bHeader = false;

   int      iHdrLen = 0;

   if (bHdrPresent)
   {
      iHdrLen = _Header1._iFinish - (_Header1._iStart + sizeof(BEGIN_OF_HEADER1) - 1);

      memcpy(pszHeader,pText + _Header1._iStart + sizeof(BEGIN_OF_HEADER1) - 1,iHdrLen);

      pszHeader[iHdrLen] = 0;  // ASCIIZ

      Jammer(pszHeader,iHdrLen);

      pszHeader[iHdrLen] = 0; // ASCIIZ

      bHeader = iHdrLen > 0  ?  true  :  false;
   }

   if (bTitlePresent && bHdrPresent)
   {
      // Skip Title
      WriteBuffer(hFile,pText,_Title._iStart);
      WriteBuffer(hFile,pText + _Title._iFinish + sizeof(END_OF_TITLE) - 1,_Head._iFinish - (_Title._iFinish + sizeof(END_OF_TITLE) - 1));
   }
   else
   {
      WriteBuffer(hFile,pText,_Head._iFinish);
   }
          
   if (bHeader)
   {
      WriteBuffer(hFile,EOL_MARKER,sizeof(EOL_MARKER) - 1);
      WriteBuffer(hFile,BEGIN_OF_TITLE,sizeof(BEGIN_OF_TITLE) - 1);
      WriteBuffer(hFile,pszHeader,iHdrLen);
      WriteBuffer(hFile,END_OF_TITLE,sizeof(END_OF_TITLE) - 1);
      WriteBuffer(hFile,EOL_MARKER,sizeof(EOL_MARKER) - 1);
   }
   
   WriteBuffer(hFile,pText + _Head._iFinish,dwSize - _Head._iFinish);

   MF.Close();

   CloseHandle(hFile);
   hFile = INVALID_HANDLE_VALUE;
}

/* ******************************************************************** **
** @@ ShowHelp()
** @  Copyrt :
** @  Author :
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

void ShowHelp()
{
   const char  pszCopyright[] = "-*-   Title 1.0   *   Copyright (c) gazlan, 2011   -*-";
   const char  pszDescript [] = "Create/Replace HTML <title> tag with first occurrent of <h1>content</h1>";
   const char  pszE_Mail   [] = "complains_n_suggestions direct to gazlan@yandex.ru";

   printf("%s\n\n",pszCopyright);
   printf("%s\n\n",pszDescript);
   printf("Usage: title.com wildcards [ > LOG.TXT ]\n\n");
   printf("%s\n\n",pszE_Mail);
}

/* ******************************************************************** **
** @@ main()
** @  Copyrt :
** @  Author :
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

int main(int argc,char**argv)
{
   if (argc < 2)
   {
      ShowHelp();
      return 0;
   }

   if (argc == 2)
   {
      if ((!strcmp(argv[1],"?")) || (!strcmp(argv[1],"/?")) || (!strcmp(argv[1],"-?")) || (!stricmp(argv[1],"/h")) || (!stricmp(argv[1],"-h")))
      {
         ShowHelp();
         return 0;
      }
   }

   char  pszMask[MAX_PATH + 1];

   strncpy(pszMask,argv[1],MAX_PATH);
   pszMask[MAX_PATH] = 0; // ASCIIZ

   char     pszDrive   [_MAX_DRIVE];
   char     pszDir     [_MAX_DIR];
   char     pszFName   [_MAX_FNAME];
   char     pszExt     [_MAX_EXT];

   _splitpath(pszMask,pszDrive,pszDir,pszFName,pszExt);

   char     pszSrchMask[MAX_PATH + 1];
   char     pszSrchPath[MAX_PATH + 1];

   strcpy(pszSrchMask,pszFName);
   strcat(pszSrchMask,pszExt);

   Walker      Visitor;

   Visitor.Init(ForEach,pszSrchMask,false);

   strcpy(pszSrchPath,pszDrive);
   strcat(pszSrchPath,pszDir);

   Visitor.Run(*pszSrchPath  ?  pszSrchPath  :  ".");

   return 0;
}

/* ******************************************************************** **
** @@                   End of File
** ******************************************************************** */
