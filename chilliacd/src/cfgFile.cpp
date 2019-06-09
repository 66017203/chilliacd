// ------------------------------------------------------------------------
//  Copyright (c) 2000 Carsten Breuer
/************************************************************************/

// Standard Lib

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cfgFile.h"

/* DONT_HAVE_STRUPR is set when INI_REMOVE_CR is defined */
#ifdef DONT_HAVE_STRUPR
void _strupr(char *str)
{
}
#endif


void cfg_RemoveQuotation(char *string)  
{
	if( !string || !string[0])
		return ;
	
	char *p =   string + strlen(string) -1 ;
	
	if ((*p == '\'' && *string == '\'') ||
		(*p == '\"' && *string == '\"'))
	{
		*p = 0;
		for(p = string+1; *p; p++ )
			*string++ = *p ;
		*string = 0 ;
	}
}

/*=========================================================================
   CTIniFile : The constructor
*========================================================================*/
CTCfgFile::CTCfgFile (void)
{
	m_pEntry      = NULL;
	m_pCurEntry   = NULL;
	m_result [0]  = 0;
	m_pIniFile    = NULL;
}

/*=========================================================================
   CTIniFile : The destructor
*========================================================================*/
CTCfgFile::~CTCfgFile (void)
{
	// FreeAllMem ();
    Close();
}

/*=========================================================================
   CTIniFile : GetVersion
   Info     : The version is BCD coded. It maintain the major version in
              the upper 8 bits and the minor in the lower.
              0x0120 means version 1.20
*========================================================================*/
uint32_t CTCfgFile::GetVersion (void)
{
	return 0x0030;
}



/*=========================================================================
   OpenIniFile
  -------------------------------------------------------------------------
   Job : Opens an ini file or creates a new one if the requested file
         doesnt exists.
========================================================================*/
int32_t CTCfgFile::Open(const char * FileName)
{
	char   Str [255];
	char   *pStr;
	struct CFGIENTRY *pEntry;

	FreeAllMem ();

	if (FileName == NULL)                             { return CT_intFALSE; }
	if( !strcmp( FileName, "" ) )				      { return CT_intFALSE; }
	if ((m_pIniFile = fopen (FileName, "r")) == NULL) { return CT_intFALSE; }

	while (fgets (Str, 255, m_pIniFile) != NULL)
	{
		pStr = strchr (Str, '\n');
		if (pStr != NULL) { *pStr = 0; }
		pEntry = MakeNewEntry ();
		if (pEntry == NULL) { return CT_intFALSE; }

		#ifdef INI_REMOVE_CR
		int Len = strlen(Str);
		if ( Len > 0 )
		{
			if ( Str[Len-1] == '\r' )
			{
				Str[Len-1] = '\0';
			}
		}
		#endif

//		pEntry->pText = (char *)malloc (strlen (Str)+1);
		pEntry->pText = new char[strlen(Str)+1];
		if (pEntry->pText == NULL)
		{
			FreeAllMem ();
			return CT_intFALSE;
		}
		strcpy (pEntry->pText, Str);
		pStr = strchr (Str,';');
		if (pStr != NULL) { *pStr = 0; } /* Cut all comments */
		pStr = strchr (Str,'#');
		if (pStr != NULL) { *pStr = 0; } /* Cut all comments */
		pStr = strchr (Str,'/');
		if (pStr != NULL && pStr[1] == '/' )
			{ *pStr = 0; } /* Cut all comments */

		if ( (strstr (Str, "[") > 0) && (strstr (Str, "]") > 0) ) /* Is Section */
		{
			pEntry->Type = tpSECTION;

			pStr = strchr (Str, ']');  //zsw add 
			if( pStr != NULL ) *pStr = 0 ;
			pStr = strchr( Str, '[' );
			if( pStr ) strcpy( pEntry->pText, pStr+1);
			alltrim(pEntry->pText);
		}
		else
		{
			if (strstr (Str, "=") > 0)
			{
				pEntry->Type = tpKEYVALUE;
			}
			else
			{
				pEntry->Type = tpCOMMENT;
			}
		}
		m_pCurEntry = pEntry;
	}
	fclose (m_pIniFile);
	m_pIniFile = NULL;
	return CT_intTRUE;
}

/*=========================================================================
   CloseIniFile
  -------------------------------------------------------------------------
   Job : Closes the ini file without any modifications. If you want to
         write the file use WriteIniFile instead.
========================================================================*/
void CTCfgFile::Close(void)
{
	FreeAllMem ();
	if (m_pIniFile != NULL)
	{
		fclose (m_pIniFile);
		m_pIniFile = NULL;
	}
}


/*=========================================================================
   WriteIniFile
  -------------------------------------------------------------------------
   Job : Writes the iniFile to the disk and close it. Frees all memory
         allocated by WriteIniFile;
========================================================================*/
int32_t CTCfgFile::WriteIniFile (const char *pFileName)
{
	struct CFGIENTRY *pEntry = m_pEntry;
	if (m_pIniFile != NULL)
	{
		fclose (m_pIniFile);
	}
	if ((m_pIniFile = fopen (pFileName, "wb")) == NULL)
	{
		FreeAllMem ();
		return CT_intFALSE;
	}

	while (pEntry != NULL)
	{
		if (pEntry->Type != tpNULL)
		{
#ifdef INI_REMOVE_CR
            /* ?? */
            if ( pEntry->Type == tpSECTION )
            {
                fprintf (m_pIniFile, "[%s]\n", pEntry->pText );
            }
            else
            {
			    fprintf (m_pIniFile, "%s\n", pEntry->pText);
            };
#else
			
            if ( pEntry->Type == tpSECTION )
            {
                fprintf (m_pIniFile, "[%s]\r\n", pEntry->pText );
            }
            else
            {
			    fprintf (m_pIniFile, "%s\r\n", pEntry->pText);
            };
#endif
		}
		pEntry = pEntry->pNext;
	}

	fclose (m_pIniFile);
	m_pIniFile = NULL;
	return CT_intTRUE;
}


/*=========================================================================
   WriteString : Writes a string to the ini file
========================================================================*/
void CTCfgFile::WriteString (const char *pSection, const char *pKey, const char *pValue)
{
	CFGEFIND List;
	char  Str [255] = {0};

	if (ArePtrValid (pSection, pKey, pValue) == CT_intFALSE) { return; }
	if (FindKey  (pSection, pKey, &List) == CT_intTRUE)
	{
		sprintf (Str, "%s=%s%s", List.KeyText, pValue, List.Comment);
		FreeMem (List.pKey->pText);
//		List.pKey->pText = (char *)malloc (strlen (Str)+1);
		List.pKey->pText = new char[strlen (Str)+1];
		strcpy (List.pKey->pText, Str);
	}
	else
	{
		if ((List.pSec != NULL) && (List.pKey == NULL)) /* section exist, Key not */
		{
			AddKey (List.pSec, pKey, pValue);
		}
		else
		{
			AddSectionAndKey (pSection, pKey, pValue);
		}
	}
}

/*=========================================================================
   WriteBool : Writes a boolean to the ini file
*========================================================================*/
void CTCfgFile::WriteBool (const char *pSection, const char *pKey, int32_t Value)
{
	char Val [2] = {'0',0};
	if (Value != 0) { Val [0] = '1'; }
	WriteString(pSection, pKey, Val);
}

/*=========================================================================
   WriteInt : Writes an integer to the ini file
*========================================================================*/
void CTCfgFile::WriteInt (const char *pSection, const char *pKey, int Value)
{
	char Val [12] = {0}; /* 32bit maximum + sign + \0 */
	sprintf (Val, "%d", Value);
	WriteString(pSection, pKey, Val);
}

/*=========================================================================
   WriteDouble : Writes a double to the ini file
*========================================================================*/
void CTCfgFile::WriteDouble (const char *pSection, const char *pKey, double Value)
{
	char Val [32] = {0}; /* DDDDDDDDDDDDDDD+E308\0 */
	sprintf (Val, "%1.10lE", Value);
	WriteString(pSection, pKey, Val);
}


/*=========================================================================
   ReadString : Reads a string from the ini file
*========================================================================*/
const char *CTCfgFile::ReadString (const char *pSection, const char *pKey, const char *pDefault)
{
	CFGEFIND List;
	if (ArePtrValid (pSection, pKey, pDefault) == CT_intFALSE) { return pDefault; }

	if (FindKey  (pSection, pKey, &List) == CT_intTRUE)
	{
		strcpy (m_result, List.ValText);
		if ( *m_result != '\0')
			return m_result;
	}
	return pDefault;
}


/*=========================================================================
   ReadBool : Reads a boolean from the ini file
*========================================================================*/
int32_t CTCfgFile::ReadBool (const char *pSection, const char *pKey, int32_t Default)
{
	char Val [2] = {"0"};
	if (Default != 0) { Val [0] = '1'; }
	return (atoi (ReadString (pSection, pKey, Val))?1:0); /* Only allow 0 or 1 */
}

/*=========================================================================
   ReadInt : Reads a integer from the ini file
*========================================================================*/
int CTCfgFile::ReadInt (const char *pSection, const char *pKey, int Default)
{
	char Val[12] = {0};
	sprintf (Val,"%d", Default);
    /*  add support to read HEX integers, by ZPX.   */
	//return (atoi (ReadString (pSection, pKey, Val)));
    int     ret(0);
    char    *val_str    = (char*)ReadString(pSection, pKey, Val);
    if ( strlen(val_str) > 2 && val_str[0] == '0' && val_str[1] == 'x' )
    {
        //_strupr( (char*)(val_str + 2) );
        sscanf(val_str, "0x%x", &ret );
    }
    else
    {
        ret = atoi( val_str );
    };

    return ret;
}

/*=========================================================================
   ReadDouble : Reads a double from the ini file
*========================================================================*/
double CTCfgFile::ReadDouble (const char *pSection, const char *pKey, double Default)
{
    double Val = Default;
    m_result[0] = '\0';
    sscanf (ReadString (pSection, pKey, m_result), "%lE", &Val);
    return Val;
}

/*=========================================================================
   DeleteKey : Deletes an entry from the ini file
*========================================================================*/
int32_t CTCfgFile::DeleteKey (const char *pSection, const char *pKey)
{
	CFGEFIND         List;
	struct CFGIENTRY *pPrev;
	struct CFGIENTRY *pNext;

	if (FindKey (pSection, pKey, &List) == CT_intTRUE)
	{
		pPrev = List.pKey->pPrev;
		pNext = List.pKey->pNext;
		if (pPrev)
		{
			pPrev->pNext=pNext;
		}
		if (pNext)
		{ 
			pNext->pPrev=pPrev;
		}
		FreeMem (List.pKey->pText);
		FreeMem (List.pKey);
		return CT_intTRUE;
	}
	return CT_intFALSE;
}




/* Here we start with our helper functions */

void CTCfgFile::FreeMem (const char *pPtr)
{
	if (pPtr != NULL) 
	{ 
		delete pPtr; 
	}
}

void CTCfgFile::FreeMem(const CFGIENTRY * pPtr)
{
	delete pPtr;
}

void CTCfgFile::FreeAllMem (void)
{
	struct CFGIENTRY *pEntry;
	struct CFGIENTRY *pNextEntry;
	pEntry = m_pEntry;
	while (1)
	{
		if (pEntry == NULL) { break; }
		pNextEntry = pEntry->pNext;
		delete[] pEntry->pText;//FreeMem (pEntry->pText); /* Frees the pointer if not NULL */
		FreeMem (pEntry);
		pEntry = pNextEntry;
	}
	m_pEntry    = NULL;
	m_pCurEntry = NULL;
}

struct CFGIENTRY *CTCfgFile::FindSection (const char *pSection)
{
	char Sec  [255] = {0};
	char iSec [255] = {0};
	struct CFGIENTRY *pEntry;
	strcpy( Sec, pSection); //modify by zsw
	_strupr  (Sec);
	pEntry = m_pEntry; /* Get a pointer to the first Entry */
	while (pEntry != NULL)
	{
		if (pEntry->Type == tpSECTION)
		{
			strcpy  (iSec, pEntry->pText);
			_strupr  (iSec);
			if (strcmp (Sec, iSec) == 0)
			{
				return pEntry;
			}
		}
		pEntry = pEntry->pNext;
	}
	return NULL;
}

int32_t	CTCfgFile::DeleteKeyOfSection (const char *pSection)
{
	char Sec  [255] = {0};
	char iSec [255] = {0};
	struct CFGIENTRY *pEntry;
	strcpy( Sec, pSection); 
	_strupr  (Sec);
	pEntry = m_pEntry; /* Get a pointer to the first Entry */
	while (pEntry != NULL)
	{				
		if (pEntry->Type == tpSECTION)//Found the section,break
		{
			strcpy  (iSec, pEntry->pText);
			_strupr  (iSec);
			if (strcmp (Sec, iSec) == 0)
			{
				break;
			}
		}
		pEntry = pEntry->pNext;
	}	
	if(pEntry!=NULL)//If the section existing
	{
		while((pEntry->pNext)!=NULL)//Delete the keyvalue of the section
		{
			if (((pEntry->pNext->Type) == tpSECTION) || 
					((pEntry->pNext->Type) == tpNULL   ))
			{
				return CT_intFALSE;
			}
			else
			{			
				struct CFGIENTRY *temp;
				temp = pEntry->pNext;			
				pEntry->pNext = temp->pNext;
				FreeMem (temp->pText);
				FreeMem (temp);	
			}
		}
	}
	return CT_intFALSE;	
}
//Add ended

//#define Min 
int32_t CTCfgFile::FindKey  (const char *pSection, const char *pKey, CFGEFIND *pList)
{
	char Search [255] = {0};
	char Found  [255] = {0};
	char Text   [255] = {0};
	char *pText, *pText2;
	struct CFGIENTRY *pEntry;
	pList->pSec        = NULL;
	pList->pKey        = NULL;
	pEntry = FindSection (pSection);
	if (pEntry == NULL) { return CT_intFALSE; }
	pList->pSec        = pEntry;
	pList->KeyText[0] = 0;
	pList->ValText[0] = 0;
	pList->Comment[0] = 0;
	pEntry = pEntry->pNext;
	if (pEntry == NULL) { return CT_intFALSE; }
	sprintf (Search, "%s",pKey);
	_strupr  (Search);
	while (pEntry != NULL)
	{
		if ((pEntry->Type == tpSECTION) || /* Stop after next section or EOF */
			(pEntry->Type == tpNULL   ))
		{
			return CT_intFALSE;
		}
		if (pEntry->Type == tpKEYVALUE)
		{
			strcpy (Text, pEntry->pText);
//			pText = strchr (Text, ';');
//			if (pText == NULL)
//				pText = strchr (Text, '#');
//			else
//			{
//				pText2 = strchr (Text, '#');
//				if (pText2 != NULL)
//					pText = Min(pText, pText2);
//			}

//ysh modify [4/15/2004]
			pText = strrchr(Text, ';');
			pText2 = strrchr(Text, '#');
			if (pText2 > pText)
				pText = pText2;

			pText2 = strrchr(Text, '/');
			if (pText2 && 
				pText2>Text &&
				pText2 > pText &&
				'/' == *(pText2-1))
				pText = pText2-1;

				
			if (pText != NULL)
			{
				strcpy (pList->Comment, pText);
				*pText = 0;
			}

			pText = strchr (Text, '=');
			if (pText != NULL)
			{
				*pText = 0;
				
				strcpy (pList->KeyText, Text);

				strcpy (Found, Text);
				alltrim(Found);  //zsw add
				*pText = '=';
				_strupr (Found);
	
				if (strcmp (Found,Search) == 0)
				{
				   strcpy (pList->ValText, pText+1);
				   alltrim(pList->ValText) ;//zsw must add 
				   
				   pList->pKey = pEntry;
				   return CT_intTRUE;
				}
			}
		}
		pEntry = pEntry->pNext;
	}
	return CT_intFALSE;
}

int32_t CTCfgFile::AddItem (char Type, const char *pText)
{
	struct CFGIENTRY *pEntry = MakeNewEntry ();
	if (pEntry == NULL) { return CT_intFALSE; }
	pEntry->Type = Type;
	pEntry->pText = new char[strlen (pText) +1];
	if (pEntry->pText == NULL)
	{
		delete (pEntry);
		return CT_intFALSE;
	}
	strcpy (pEntry->pText, pText);
	pEntry->pNext   = NULL;
	if (m_pCurEntry != NULL) { m_pCurEntry->pNext = pEntry; }
	m_pCurEntry    = pEntry;
	return CT_intTRUE;
}

int32_t CTCfgFile::AddItemAt (struct CFGIENTRY *pEntryAt, char Mode, const char *pText)
{
	struct CFGIENTRY *pNewEntry;
	if (pEntryAt == NULL)  { return CT_intFALSE; }
	pNewEntry = new CFGIENTRY ;
	if (pNewEntry == NULL) { return CT_intFALSE; }
	pNewEntry->pText = new char[strlen (pText)+1];
	if (pNewEntry->pText == NULL)
	{
		delete (pNewEntry);
		return CT_intFALSE;
	}
	strcpy (pNewEntry->pText, pText);
	if (pEntryAt->pNext == NULL) /* No following nodes. */
	{
		pEntryAt->pNext   = pNewEntry;
		pNewEntry->pNext  = NULL;
	}
	else
	{
		pNewEntry->pNext = pEntryAt->pNext;
		pEntryAt->pNext  = pNewEntry;
	}
	pNewEntry->pPrev = pEntryAt;
	pNewEntry->Type  = Mode;
	return CT_intTRUE;
}

int32_t CTCfgFile::AddSectionAndKey (const char *pSection, const char *pKey, const char *pValue)
{
	char Text [255] = {0};
    /*  by ZPX. */
	//sprintf (Text, "[%s]", pSection);
    sprintf (Text, "%s", pSection);
	if (AddItem (tpSECTION, Text) == CT_intFALSE) { return CT_intFALSE; }
	sprintf (Text, "%s = %s", pKey, pValue);
	return AddItem (tpKEYVALUE, Text)? 1 : 0;
}

void CTCfgFile::AddKey (struct CFGIENTRY *pSecEntry, const char *pKey, const char *pValue)
{
	char Text [255] = {0};
	sprintf (Text, "%s=%s", pKey, pValue);
	AddItemAt (pSecEntry, tpKEYVALUE, Text);
}

struct CFGIENTRY *CTCfgFile::MakeNewEntry (void)
{
	struct CFGIENTRY *pEntry;
	pEntry = new CFGIENTRY ;
	if (pEntry == NULL)
	{
		FreeAllMem ();
		return NULL;
	}
	if (m_pEntry == NULL)
	{
		m_pEntry = pEntry;
	}
	pEntry->Type  = tpNULL;
	pEntry->pPrev = m_pCurEntry;
	pEntry->pNext = NULL;
	pEntry->pText = NULL;
	if (m_pCurEntry != NULL)
	{
		m_pCurEntry->pNext = pEntry;
	}
	return pEntry;
}

void CTCfgFile::alltrim(char *string)  //add by zsw
{
	if( !string || !string[0])
		return ;

	char *p =   string + strlen(string) -1 ;
    /*  trim tail.  */
	for(; p >= string && (*p == ' ' || *p == '\t') ; p-- )  // add trim of TAB.
		*p = 0 ;
    /*  trim head.  */
	for( p = string; *p == ' ' || *p == '\t' ; p++ );
	for(; *p; p++ )
		*string++ = *p ;
	*string = 0 ;
}




int32_t CTCfgFile::AddSectionEx(const char * pSection,const char *lpString)
{
	char Text [255] = {0};
	struct CFGIENTRY *pEntry;
	pEntry = FindSection (pSection);//First found the section
	if (pEntry == NULL) //If the section did not exist,add new section and keyvalue
	{
		sprintf (Text, "%s", pSection);
		if (AddItem (tpSECTION, Text) == CT_intFALSE) 
		{ 
			return CT_intFALSE; 
		}
		sprintf (Text, "%s", lpString);
		if (AddItem (tpKEYVALUE, Text) == CT_intFALSE) 
		{ 
			return CT_intFALSE; 
		}
	}	
	else //If the section already exist,than replace the key of section with lpString
	{
		DeleteKeyOfSection (pSection);
		sprintf (Text, "%s", lpString);
		AddItemAt (pEntry,tpKEYVALUE, Text);
	}
	return 	CT_intTRUE;
}
//Add end
////////////////////////////////


int32_t cfg_GetPrivateProfileBoolEx(const char *pSection, const char *pKey, int32_t   Default, const char *pFileName)
{
	CTCfgFile ini ;
	ini.Open(pFileName);
	int32_t ret = ini.ReadBool( pSection, pKey, Default);
	ini.Close();
	return ret;
}

int cfg_GetPrivateProfileIntEx(const char *pSection, const char *pKey, int    Default, const char *pFileName)
{
	CTCfgFile ini ;
	ini.Open(pFileName);
	int ret = ini.ReadInt( pSection, pKey, Default);
	ini.Close();
	return ret;
}

double cfg_GetPrivateProfileDoubleEx  (const char *pSection, const char *pKey, double Default, const char *pFileName)
{
	CTCfgFile ini ;
	ini.Open(pFileName);
	double ret = ini.ReadDouble( pSection, pKey, Default);
	ini.Close();
	return ret;
}

char    *cfg_GetPrivateProfileStringEx (const char *pSection, const char *pKey, const char  *pDefault, char *pReturnedString,  int nSize, const char *pFileName)
{
	CTCfgFile ini ;
	ini.Open(pFileName);
	if (pDefault == NULL)
		strncpy( pReturnedString, ini.ReadString( pSection, pKey, ""), nSize) ;
	else
		strncpy( pReturnedString, ini.ReadString( pSection, pKey, pDefault), nSize) ;

	ini.Close();
	cfg_RemoveQuotation(pReturnedString);
	return pReturnedString ;
}

void    cfg_WritePrivateProfileBoolEx   (const char *pSection, const char *pKey, int32_t   Value, const char *pFileName)
{
	CTCfgFile ini ;
	ini.Open(pFileName);
	ini.WriteBool( pSection, pKey, Value);
	ini.WriteIniFile (pFileName);
	ini.Close();
}

void    cfg_WritePrivateProfileIntEx    (const char *pSection, const char *pKey, int    Value, const char *pFileName)
{
	CTCfgFile ini ;
	ini.Open(pFileName);
	ini.WriteInt( pSection, pKey, Value);
	ini.WriteIniFile (pFileName);
	ini.Close();
}

void    cfg_WritePrivateProfileDoubleEx (const char *pSection, const char *pKey, double Value, const char *pFileName)
{
	CTCfgFile ini ;
	ini.Open(pFileName);
	ini.WriteDouble( pSection, pKey, Value);
	ini.WriteIniFile (pFileName);
	ini.Close();
}

void    cfg_WritePrivateProfileStringEx (const char *pSection, const char *pKey, const char  *pValue, const char *pFileName)
{
	CTCfgFile ini ;
	ini.Open(pFileName);
	ini.WriteString( pSection, pKey, pValue);
	ini.WriteIniFile (pFileName);
	ini.Close();
}

//Add by c.s.wang
unsigned int cfg_GetPrivateProfileSectionNamesEx(char * strReturn,
			unsigned int nSize,const char * lpFileName)
{
	char   Str [255] = {0};
	char tempstr[255] = {0};
	FILE *m_pIniFile;
	char   *pStr;
	unsigned int reResult=0;
	if( !strcmp( lpFileName, "" ) )
	{ 
		return CT_intFALSE; 
	}
	if ((m_pIniFile = fopen (lpFileName, "r")) == NULL) 
	{
		 return CT_intFALSE; 
	}
	
	while (fgets (Str, 255, m_pIniFile) != NULL)
	{
		pStr = strchr (Str, '\n');
		if (pStr != NULL) { *pStr = 0; }
		
		pStr = strchr (Str,';');
		if (pStr != NULL) { *pStr = 0; } /* Cut all comments */
		pStr = strchr (Str,'#');
		if (pStr != NULL) { *pStr = 0; } /* Cut all comments */
		pStr = strchr (Str,'/');
		if (pStr != NULL && pStr[1] == '/' )
			{ *pStr = 0; } /* Cut all comments */
		if ( (strstr (Str, "[") > 0) && (strstr (Str, "]") > 0) ) /* Is Section */
		{
			pStr = strchr (Str, ']');
			if( pStr != NULL ) *pStr = 0 ;
			pStr = strchr( Str, '[' );
			if( pStr )
			{
				strcpy( tempstr, pStr+1);
				//alltrim(tempstr);
				for(int i=0;tempstr[i]!=0;i++)
				{
					*strReturn++=tempstr[i];
					reResult++;
				}
				*strReturn++='\0';
			}
		}
	}
	*strReturn++='\0';
	if(reResult>=nSize)
	{
		reResult=nSize;
	}
	fclose (m_pIniFile);
	m_pIniFile = NULL;
	//free(tempstr);
	return reResult;
}


int cfg_WritePrivateProfileSectionEx(const char * pSection,
				const char *lpString,const char * lpFileName)
{
	CTCfgFile ini ;
	ini.Open(lpFileName);
	ini.AddSectionEx(pSection,lpString);
	ini.WriteIniFile (lpFileName);
	ini.Close();
	return CT_intTRUE;
}
//Add ended
