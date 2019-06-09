#ifndef CFGFILE_H
#define CFGFILE_H

#include "stdio.h"   // for FILE
#include <stdint.h>
#ifndef WIN32 /* Remove CR, on unix systems. */
#define INI_REMOVE_CR
#define DONT_HAVE_STRUPR
#endif

#define tpNULL       0
#define tpSECTION    1
#define tpKEYVALUE   2
#define tpCOMMENT    3

#define CT_intTRUE         1
#define CT_intFALSE        0

struct CFGIENTRY
{
   char   Type;
   char  *pText;
   struct CFGIENTRY *pPrev;
   struct CFGIENTRY *pNext;
};

typedef struct
{
   struct CFGIENTRY *pSec;
   struct CFGIENTRY *pKey;
   char          KeyText [128];
   char          ValText [128];
   char          Comment [255];
} CFGEFIND;

/* Macros */
#define ArePtrValid(Sec,Key,Val) ((Sec!=NULL)&&(Key!=NULL)&&(Val!=NULL))

class CTCfgFile
{
public:
    CTCfgFile    (void);
    ~CTCfgFile   (void);
    uint32_t  GetVersion  (void);
    int32_t  Open(const char *pFileName);
    int32_t  ReadBool    (const char *pSection, const char *pKey, int32_t   Default);
    int     ReadInt     (const char *pSection, const char *pKey, int    Default);
    double  ReadDouble  (const char *pSection, const char *pKey, double Default);
    const char    *ReadString (const char *pSection, const char *pKey, const char  *pDefault);

    void    WriteBool   (const char *pSection, const char *pKey, int32_t   Value);
    void    WriteInt    (const char *pSection, const char *pKey, int    Value);
    void    WriteDouble (const char *pSection, const char *pKey, double Value);
    void    WriteString (const char *pSection, const char *pKey, const char  *pValue);

    void    Close();
    int32_t  WriteIniFile (const char *pFileName);
	int32_t	DeleteKey (const char *pSection, const char *pKey);
    int32_t	DeleteKeyOfSection (const char *pSection);//Add by c.s.wang
    int32_t	AddSectionEx(const char * pSection,const char *lpString); //Add by c.s.wang

protected:
	struct  CFGIENTRY *m_pEntry;
	struct  CFGIENTRY *m_pCurEntry;
	char    m_result [255];
	FILE    *m_pIniFile;
	void    AddKey     (struct CFGIENTRY *pEntry, const char *pKey, const char *pValue);
	int32_t  AddItem    (char Type, const char *pText);
	int32_t  AddItemAt (struct CFGIENTRY *pEntryAt, char Mode, const char *pText);
	void    FreeMem    (const char *pPtr);
	void	FreeMem(const CFGIENTRY * pPtr);
	void    FreeAllMem (void);
	int32_t  FindKey    (const char *pSection, const char *pKey, CFGEFIND *pList);
	int32_t  AddSectionAndKey (const char *pSection, const char*pKey, const char*pValue);
	struct  CFGIENTRY *MakeNewEntry (void);
	struct  CFGIENTRY *FindSection (const char *pSection);

private:
	void    alltrim(char *string); //add by zsw

};

////////////////////////////////////////////////////////////////////////

int32_t  cfg_GetPrivateProfileBoolEx    (const char *pSection, const char *pKey, int32_t   Default, const char *pFileName);
int     cfg_GetPrivateProfileIntEx     (const char *pSection, const char *pKey, int    Default, const char *pFileName);
double  cfg_GetPrivateProfileDoubleEx  (const char *pSection, const char *pKey, double Default, const char *pFileName);
char    *cfg_GetPrivateProfileStringEx (const char *pSection, const char *pKey, const char  *pDefault, char *pReturnedString,  int nSize, const char *pFileName);
void    cfg_WritePrivateProfileBoolEx   (const char *pSection, const char *pKey, int32_t   Value, const char *pFileName);
void    cfg_WritePrivateProfileIntEx    (const char *pSection, const char *pKey, int    Value, const char *pFileName);
void    cfg_WritePrivateProfileDoubleEx (const char *pSection, const char *pKey, double Value, const char *pFileName);
void    cfg_WritePrivateProfileStringEx (const char *pSection, const char *pKey, const char  *pValue, const char *pFileName);
unsigned int cfg_GetPrivateProfileSectionNamesEx(char * strReturn,
				unsigned int nSize,const char * lpFileName);
int cfg_WritePrivateProfileSectionEx(const char * pSection,	const char *lpString,const char * lpFileName);//Add by c.s.wang
#endif


