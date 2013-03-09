#include "stdafx.h"
#include <string.h>
#include <stdio.h>
#include "file.h"
#include "xutil.h"
#ifdef __SECURITY_0628
#ifdef __CLIENT
#include "definetext.h"
#endif	// __CLIENT
#endif	// __SECURITY_0628
#include "tools.h"

#ifdef __BSXPACK
#include <vector>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace boost;
using namespace boost::iostreams;
namespace io = boost::iostreams;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFileIO

BOOL CFileIO::Close(void)
{
	if( fp ) 
	{ 
		fclose( fp ); 
		fp = 0; 
		return TRUE; 
	}
	return FALSE;
}

BOOL CFileIO::Open( LPCTSTR fileName, TCHAR* mode )
{
	fp = _tfopen( fileName, mode );
	if( fp )
		return TRUE;
	return FALSE;
}

LPVOID CFileIO::Read(void)
{
	long size = GetLength();
	LPVOID ptr = new BYTE[ size ];
	if( !ptr ) 
		return NULL;

	fseek( fp, 0, SEEK_SET );
	if( fread( ptr, size, 1, fp ) ) 
	{ 
		return ptr;
	}
	safe_delete_array( ptr ); 
	return NULL;
}
int CFileIO::PutWideString( LPCTSTR lpszString )
{
	wchar_t wszBuff[ 1024 ];
	int nLen = strlen( lpszString );

	ATL::_mbstowcsz(wszBuff, lpszString,nLen + 1 );

	return Write( wszBuff, wcslen( wszBuff ) * 2 );
}

#ifdef __BSXPACK_OBJ
void CFileIO::ReadAll()
{
	
}

void CFileIO::ReadPart(void *ptr, size_t size, size_t n)
{
	this->Read(ptr, size, n);
}
#endif

#if defined( __CLIENT )
/////////////////////////////////////////////////////////////////////////////
// CResFile
CMapStringToPtr CResFile::m_mapResource;

#ifdef __SECURITY_0628
char	CResFile::m_szResVer[100];
map<string, string>	CResFile::m_mapAuth;
#endif	// __SECURITY_0628

CResFile::CResFile( LPCTSTR lpszFileName, TCHAR *mode ) 
{ 
	m_bResouceInFile = FALSE;
	m_nFileSize = 0;
	m_nFileBeginPosition = 0;
	m_nFileCurrentPosition = 0;
	m_nFileEndPosition = 0;
#ifndef __BSXPACK
	m_bEncryption = FALSE;  
#endif
	ZeroMemory( m_szFileName, sizeof( m_szFileName ) );
	Open( lpszFileName, mode ); 
}
CResFile::CResFile() 
{ 
	m_bResouceInFile = FALSE;
	m_nFileSize = 0;
	m_nFileBeginPosition = 0;
	m_nFileCurrentPosition = 0;
	m_nFileEndPosition = 0;
#ifndef __BSXPACK
	m_bEncryption = FALSE;  
#endif
	ZeroMemory( m_szFileName, sizeof( m_szFileName ) );
}
 
BOOL CResFile::Close( void )
{
	if( !m_bResouceInFile )
		return CFileIO::Close();
	else
	{
		if(this->TmpPtr)
		{
			this->TmpPtr = 0;
			safe_delete_array(this->TmpPtr);
		}
		if( INVALID_HANDLE_VALUE != ( HANDLE )m_File.m_hFile )
			m_File.Close();
		return TRUE;
	}
}

#ifdef __BSXPACK_OBJ
void CResFile::ReadAll()
{
	if(this->m_bResouceInFile)
	{
		this->TmpPtr = (char*)this->Read();
		this->TmpPtrPosition = 0;
	}
}

void CResFile::ReadPart(void *ptr, size_t size, size_t n)
{
	if(this->m_bResouceInFile)
	{
		char* ctptr = (char*)ptr;
		copy(&this->TmpPtr[this->TmpPtrPosition], &this->TmpPtr[this->TmpPtrPosition + (size * n)], ctptr);
		ptr = (void*)ctptr;

		ctptr = 0;
		safe_delete_array(ctptr);

		this->TmpPtrPosition += (size * n);
	}
	else
		this->Read(ptr, size, n);
}
#endif

#ifdef __BSXPACK
void CResFile::AddResource( TCHAR* lpszResName )
{
	TCHAR szFullFileName[ MAX_PATH ];
	TCHAR drive[_MAX_DRIVE], dir[_MAX_DIR], name[ _MAX_FNAME ], ext[_MAX_EXT];
	_splitpath( lpszResName, drive, dir, name, ext );

	CFile file;
	if( file.Open( lpszResName, CFile::modeRead ) == FALSE )
		return;

	int FileCount = 0;
	int Version = 0;
	int HashSize = 0;
	int TagSize = 0;

	file.Read(&FileCount, sizeof(FileCount));
	file.Read(&Version, sizeof(Version));

	file.Read(&HashSize, sizeof(HashSize));
	file.Seek(HashSize, CFile::current);

	file.Read(&TagSize, sizeof(TagSize));
	file.Seek(TagSize, CFile::current);

	for(int i = 0; i < FileCount; i++)
	{
		RESOURCE* lpRes = new RESOURCE;
		ZeroMemory(lpRes, sizeof(RESOURCE));

		strcpy( lpRes->szResourceFile, lpszResName );
		strlwr( lpRes->szResourceFile );

		int NameSize = 0;
		file.Read(&NameSize, sizeof(NameSize));

		char* Name = new char[NameSize+1];
		file.Read(Name, NameSize);
		Name[NameSize] = '\0';

		int FileHashSize = 0;
		file.Read(&FileHashSize, sizeof(FileHashSize));
		file.Seek(FileHashSize, CFile::current);

		int FileTagSize = 0;
		file.Read(&FileTagSize, sizeof(FileTagSize));
		file.Seek(FileTagSize, CFile::current);

		int CompressedFileSize = 0;
		file.Read(&CompressedFileSize, sizeof(CompressedFileSize));
		int DecompressedFileSize = 0;
		file.Read(&DecompressedFileSize, sizeof(DecompressedFileSize));

		lpRes->dwOffset = file.GetPosition();
		file.Seek(CompressedFileSize, CFile::current);
		lpRes->dwFileSize = CompressedFileSize;
		lpRes->dwDecompressedSize = DecompressedFileSize;

		strcpy(szFullFileName, dir);
		strcat(szFullFileName, Name);
		strlwr(szFullFileName);

		//::OUTPUTDEBUGSTRING("%s %s %d %d %d\n", lpRes->szResourceFile, szFullFileName, lpRes->dwOffset, lpRes->dwFileSize, lpRes->dwDecompressedSize);

		m_mapResource.SetAt( szFullFileName, lpRes );
		ZeroMemory(Name, sizeof(Name));
	}
}
#else
void CResFile::AddResource( TCHAR* lpszResName )
{
	//m_mapResource.Add( )
	int nFileHeaderSize = 0;
	int nFileNumber = 0;
	short	nFileNameLength = 0;
	char szFileName[_MAX_FNAME] = "";
	int nFileSize = 0;
	int nFilePosition = 0;
	BYTE byEncryptionKey;
	bool bEncryption;
	TCHAR szFullFileName[ MAX_PATH ];
	TCHAR drive[_MAX_DRIVE], dir[_MAX_DIR], name[ _MAX_FNAME ], ext[_MAX_EXT];
	_splitpath( lpszResName, drive, dir, name, ext );
	
	CFile file;
	if( file.Open( lpszResName, CFile::modeRead ) == FALSE )
		return;
	file.Read( &byEncryptionKey, sizeof( byEncryptionKey ) );
	file.Read( &bEncryption, sizeof( bEncryption ) );
	file.Read( &nFileHeaderSize, sizeof( int ) );
	
	char *pHeader = new char[ nFileHeaderSize ];
	// 해더는 암호 기본 
	//if( m_bEncryption ) // 암호화 되었다면
	{
		char *pCryptHeader = new char[ nFileHeaderSize ];
		file.Read( pCryptHeader, nFileHeaderSize );
		
		for( int i = 0; i < nFileHeaderSize; i++ )
		{
			pHeader[i] = Decryption( byEncryptionKey, pCryptHeader[ i ] );
		}
		
		safe_delete_array( pCryptHeader );
	}
	//else // 암호화 안되었다면
	//{
	//	m_File.Read( pHeader, nFileHeaderSize );
	//}
	int nHeaderPosition = 0;
	char strVersion[8] ="";
	memcpy( strVersion, &pHeader[ nHeaderPosition ], 7 ); nHeaderPosition += 7;
	
	memcpy( &nFileNumber, &pHeader[ nHeaderPosition ], sizeof( short ) ); nHeaderPosition += sizeof( short );
	time_t time_;
	BOOL bFind = FALSE;
	for( int i = 0; i < nFileNumber; i++ )
	{
		memcpy( &nFileNameLength, &pHeader[ nHeaderPosition ], sizeof( short ) ); nHeaderPosition += sizeof( short );
		memcpy( szFileName, &pHeader[ nHeaderPosition ], nFileNameLength ); nHeaderPosition += nFileNameLength;
		memcpy( &nFileSize, &pHeader[ nHeaderPosition ], sizeof( int ) ); nHeaderPosition += sizeof( int );
		memcpy( &time_, &pHeader[ nHeaderPosition ], sizeof( time_t ) ); nHeaderPosition += sizeof( time_t );
		memcpy( &nFilePosition, &pHeader[ nHeaderPosition ], sizeof( int ) ); nHeaderPosition += sizeof( int );
		RESOURCE* lpRes = new RESOURCE;
		ZeroMemory( lpRes, sizeof( RESOURCE ) );
		lpRes->dwOffset = nFilePosition;
		lpRes->dwFileSize = nFileSize;
		lpRes->byEncryptionKey = byEncryptionKey;
		lpRes->bEncryption = bEncryption;
		strcpy( lpRes->szResourceFile, lpszResName );
		strlwr( lpRes->szResourceFile );

		strcpy( szFullFileName, dir );
		strcat( szFullFileName, szFileName );
		strlwr( szFullFileName );


#ifdef __CLIENT
#ifdef __SECURITY_0628
		CString strFullFileName	= szFullFileName;
		if( strFullFileName.Find( "\\", 0 ) < 0 )
		{
			RESOURCE* ptr;
			if( m_mapResource.Lookup( szFullFileName, (void*&)ptr ) )
			{
				::Error( "killed by CResFile::AddResource() %s, %s, 1", prj.GetText(TID_GAME_RESOURCE_MODIFIED), szFullFileName );
				ExitProcess( -1 );
			}
		}
#endif	// __SECURITY_0628
#endif	// __CLIENT
		m_mapResource.SetAt( szFullFileName, lpRes );

		/*
		// 찾았다면
		if( 0 == strcmpi( lpszFileName, szFileName ) )
		{
			bFind = TRUE;
			//strcpy( m_szFileName, szFileName );
			//m_nFileSize = nFileSize;
			//m_nFileBeginPosition = nFilePosition;
			//m_nFileCurrentPosition = nFilePosition;
			//m_nFileEndPosition = nFilePosition + nFileSize;
			//m_File.Seek( m_nFileCurrentPosition, CFile::begin );
			break;
		}
		*/
		ZeroMemory( szFileName, sizeof( szFileName ) );
	}
	safe_delete_array( pHeader );
}
#endif

bool CompareExt( LPCTSTR szName, LPCTSTR szExt )
{
	int nExt = strlen( szExt );
	int n = strlen( szName );
	if( n < nExt )
		return false;
	return ( memcmp( szName + n - nExt, szExt, nExt ) == 0 );
}

void CResFile::ScanResource( LPCTSTR lpszRootPath )
{
	TCHAR szPath[ MAX_PATH ];
	TCHAR szPathTemp[ MAX_PATH ];
	struct _finddata_t c_file;
	long hFile;

	strcpy( szPathTemp, lpszRootPath );
	strcat( szPathTemp, "*.*" );
	if( ( hFile = _findfirst( szPathTemp, &c_file ) ) != -1L )
	{
		do
		{
			strlwr( c_file.name );
			strcpy( szPath, lpszRootPath );
			if( c_file.attrib & _A_SUBDIR )//if( CFile::GetStatus( name, fileStatus) == TRUE )
			{
				if( strcmp( c_file.name, "." ) && strcmp( c_file.name, ".." ) 
#ifdef __INTERNALSERVER
					&& strcmp( c_file.name, ".svn" )
#endif // __INTERNALSERVER
					)
				{
					strcat( szPath, c_file.name );
					strcat( szPath, "\\" );
					ScanResource( szPath );
				}
			}
			else
			//if( strstr( c_file.name, ".res" ) != 0 )
#ifdef __BSXPACK
			if(CompareExt( c_file.name, ".bsx" ))
			{
				strcat(szPath, c_file.name);
				AddResource(szPath);

			}
#else
			if(CompareExt( c_file.name, ".res" ))
			{
				strcat( szPath, c_file.name );
				AddResource( szPath  );
			}
#endif
		}
		while( _findnext( hFile, &c_file ) == 0 );
			
		_findclose( hFile );
	}	
}


void CResFile::FreeResource()
{
	CString strNameName;
	RESOURCE* lpRes;
	POSITION pos;
	pos = m_mapResource.GetStartPosition();
	while( pos )
	{
		m_mapResource.GetNextAssoc( pos, strNameName, (void*&) lpRes );
		safe_delete( lpRes );
	}
}

#ifdef __BSXPACK
BOOL CResFile::Open( LPCTSTR lpszFileName, TCHAR *mode )
{
	if( CFileIO::Open( lpszFileName, mode ) == TRUE )
	{
#ifdef __SECURITY_0628
#ifndef _DEBUG
#ifdef __MAINSERVER
		CString strFileName		= lpszFileName;
		if( strFileName.Find( "\\", 0 ) < 0 )
		{
			::Error( "killed by CResFile::Open() %s, %s, 2", prj.GetText(TID_GAME_RESOURCE_MODIFIED), lpszFileName );
			ExitProcess( -1 );
		}
#endif	// __MAINSERVER
#endif	// _DEBUG
#endif	// __SECURITY_0628
		return TRUE;
	}
	char szSerchPath[MAX_PATH];
	GetCurrentDirectory( sizeof( szSerchPath ), szSerchPath );
	TCHAR drive[_MAX_DRIVE], dir[_MAX_DIR], name[ _MAX_FNAME ], ext[_MAX_EXT];
	_splitpath( lpszFileName, drive, dir, name, ext );
	
	TCHAR szFileName[ MAX_PATH ];
	RESOURCE* lpRes;
	strcpy( szFileName, lpszFileName );
	strlwr( szFileName );
	m_bResouceInFile = FALSE;
	if( m_mapResource.Lookup( szFileName, (void*&) lpRes ) )
	{
		CFileException fileExc;
		if( m_File.Open( lpRes->szResourceFile, CFile::modeRead | CFile::shareDenyNone, &fileExc ) )
		{
			m_nFileSize = lpRes->dwFileSize;
			m_nFileBeginPosition = lpRes->dwOffset;
			m_nFileCurrentPosition = lpRes->dwOffset;
			m_nFileEndPosition = lpRes->dwOffset + m_nFileSize;
			m_File.Seek( m_nFileCurrentPosition, CFile::begin );
			m_bResouceInFile = TRUE;
#ifdef __SECURITY_0628
			lstrcpy( m_szFileName, szFileName );
#endif	// __SECURITY_0628
			//m_File.Close();

			//::Error("%s %d", szFileName, m_bResouceInFile);
			return TRUE;
		}
		
		TCHAR szCause[255];
		fileExc.GetErrorMessage(szCause, 255);

		::Error( "CResFile Open Error %s FileName = %s, Resource = %s, CurrentDir = %s\n", 
			szCause,
			szFileName, 
			lpRes->szResourceFile, 
			szSerchPath );

		return FALSE;
	}
	return FALSE;
}
#else
BOOL CResFile::Open( LPCTSTR lpszFileName, TCHAR *mode )
{
//	TRACE("CResFile::Open( %s )\n", lpszFileName );
	if( CFileIO::Open( lpszFileName, mode ) == TRUE )
	{
#ifdef __SECURITY_0628
#ifndef _DEBUG
#ifdef __MAINSERVER
		CString strFileName		= lpszFileName;
		if( strFileName.Find( "\\", 0 ) < 0 )
		{
			::Error( "killed by CResFile::Open() %s, %s, 2", prj.GetText(TID_GAME_RESOURCE_MODIFIED), lpszFileName );
			ExitProcess( -1 );
		}
#endif	// __MAINSERVER
#endif	// _DEBUG
#endif	// __SECURITY_0628
		return TRUE;
	}
	char szSerchPath[MAX_PATH];
	GetCurrentDirectory( sizeof( szSerchPath ), szSerchPath );
	TCHAR drive[_MAX_DRIVE], dir[_MAX_DIR], name[ _MAX_FNAME ], ext[_MAX_EXT];
	_splitpath( lpszFileName, drive, dir, name, ext );
	
	TCHAR szFileName[ MAX_PATH ];
	RESOURCE* lpRes;
	strcpy( szFileName, lpszFileName );
	strlwr( szFileName );
	m_bResouceInFile = FALSE;
	if( m_mapResource.Lookup( szFileName, (void*&) lpRes ) )
	{
		CFileException fileExc;
		if( m_File.Open( lpRes->szResourceFile, CFile::modeRead | CFile::shareDenyNone, &fileExc ) )
		{
			m_nFileSize = lpRes->dwFileSize;
			m_nFileBeginPosition = lpRes->dwOffset;
			m_nFileCurrentPosition = lpRes->dwOffset;
			m_nFileEndPosition = lpRes->dwOffset + m_nFileSize;
			m_byEncryptionKey = lpRes->byEncryptionKey;
			m_bEncryption = lpRes->bEncryption;
			m_File.Seek( m_nFileCurrentPosition, CFile::begin );
			m_bResouceInFile = TRUE;
#ifdef __SECURITY_0628
			lstrcpy( m_szFileName, szFileName );
#endif	// __SECURITY_0628
			//m_File.Close();
			return TRUE;
		}
		
		TCHAR szCause[255];
		fileExc.GetErrorMessage(szCause, 255);

		::Error( "CResFile Open Error %s FileName = %s, Resource = %s, CurrentDir = %s\n", 
			szCause,
			szFileName, 
			lpRes->szResourceFile, 
			szSerchPath );

		return FALSE;
	}
	return FALSE;
}
#endif

/*
BOOL CResFile::FindFile(char *szSerchPath, LPCTSTR lpszFileName, TCHAR *mode, int f )
{
	HANDLE hSrch;
	WIN32_FIND_DATA wfd;
	BOOL bResult=TRUE;
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char dir1[_MAX_DIR];
	char dir2[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	char filepath[MAX_PATH];
	TCHAR szFileName[_MAX_FNAME];	

	_splitpath( lpszFileName, drive, dir1, szFileName, ext );
	wsprintf( szFileName, "%s%s", szFileName, ext );

	_splitpath( szSerchPath, drive, dir2, fname, ext );
	if( dir1[ 0 ] == '\0' )
		wsprintf( szSerchPath, "*.res" );
	else
		wsprintf( szSerchPath, "%s\\*.res", dir1 );

	_splitpath( szSerchPath, drive, dir, fname, ext );

	if( ( hSrch = FindFirstFile( szSerchPath, &wfd ) ) != INVALID_HANDLE_VALUE )
	{
		while ( bResult ) 
		{
			_tcslwr( wfd.cFileName );
			_splitpath( wfd.cFileName, NULL, NULL, NULL, ext );
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
			{
			} 
			else 
			{
				// .Res 파일을 찾았다면
				if( 0 == strcmpi( ext, ".res" ) )
				{
					wsprintf( filepath, "%s%s%s", drive, dir, wfd.cFileName);
					// .Res 파일 안에서 원하는 파일을 찾았다면
					if( TRUE == FindFileFromResource( filepath, szFileName ) )
					{
						return TRUE;
					}
				}
			}
			bResult = FindNextFile(hSrch,&wfd);
		}
		FindClose( hSrch );
	}
	// 찾는 파일이 외부에 있다면
	//if( 0 == strcmpi( szFileName, wfd.cFileName ) )
	//{
	m_bResouceInFile = FALSE;
	return CFileIO::Open( lpszFileName, mode );
}


BOOL CResFile::FindFileFromResource( char *filepath, LPCTSTR lpszFileName )
{
	int nFileHeaderSize = 0;
	int nFileNumber = 0;
	int nFileNameLength = 0;
	char szFileName[_MAX_FNAME] = "";
	int nFileSize = 0;
	int nFilePosition = 0;
	if( m_File.Open( filepath, CFile::modeRead ) == FALSE )
		return FALSE;
	m_File.Read( &m_byEncryptionKey, sizeof( m_byEncryptionKey ) );
	m_File.Read( &m_bEncryption, sizeof( m_bEncryption ) );
	m_File.Read( &nFileHeaderSize, sizeof( int ) );

	char *pHeader = new char[ nFileHeaderSize ];
	// 해더는 암호 기본 
	//if( m_bEncryption ) // 암호화 되었다면
	{
		char *pCryptHeader = new char[ nFileHeaderSize ];
		m_File.Read( pCryptHeader, nFileHeaderSize );

		for( int i = 0; i < nFileHeaderSize; i++ )
		{
			pHeader[i] = Decryption( m_byEncryptionKey, pCryptHeader[ i ] );
		}
 
		delete [] pCryptHeader;
	}
	//else // 암호화 안되었다면
	//{
	//	m_File.Read( pHeader, nFileHeaderSize );
	//}
	int nHeaderPosition = 0;
	char strVersion[8] ="";
	memcpy( strVersion, &pHeader[ nHeaderPosition ], 7 ); nHeaderPosition += 7;
	//FILEOUT( "c:\\debug.txt", "1 %s %s %d \n", lpszFileName, strVersion, nHeaderPosition );

	memcpy( &nFileNumber, &pHeader[ nHeaderPosition ], sizeof( short ) ); nHeaderPosition += sizeof( short );
	//	FILEOUT( "c:\\debug.txt", "1 -1 %s %s %d \n", lpszFileName, strVersion, nFileNumber );
	time_t time_;
	for( int i = 0; i < nFileNumber; i++ )
	{
		//FILEOUT( "c:\\debug.txt", "2 %s %s %d \n", lpszFileName, szFileName, nFileNumber );

		memcpy( &nFileNameLength, &pHeader[ nHeaderPosition ], sizeof( short ) ); nHeaderPosition += sizeof( short );
		//FILEOUT( "c:\\debug.txt", "3 %s %s %d \n", lpszFileName, szFileName, nFileNameLength );
		memcpy( szFileName, &pHeader[ nHeaderPosition ], nFileNameLength ); nHeaderPosition += nFileNameLength;
		//FILEOUT( "c:\\debug.txt", "4 %s %s \n", lpszFileName, szFileName );
		memcpy( &nFileSize, &pHeader[ nHeaderPosition ], sizeof( int ) ); nHeaderPosition += sizeof( int );
		memcpy( &time_, &pHeader[ nHeaderPosition ], sizeof( time_t ) ); nHeaderPosition += sizeof( time_t );
		//FILEOUT( "c:\\debug.txt", "5 %s %s \n", lpszFileName, szFileName );
		memcpy( &nFilePosition, &pHeader[ nHeaderPosition ], sizeof( int ) ); nHeaderPosition += sizeof( int );
		//FILEOUT( "c:\\debug.txt", "6 %s %s \n", lpszFileName, szFileName );
		// 찾았다면
		if( 0 == strcmpi( lpszFileName, szFileName ) )
		{
			m_bResouceInFile = TRUE;
			strcpy( m_szFileName, szFileName );
			m_nFileSize = nFileSize;
			m_nFileBeginPosition = nFilePosition;
			m_nFileCurrentPosition = nFilePosition;
			m_nFileEndPosition = nFilePosition + nFileSize;
			m_File.Seek( m_nFileCurrentPosition, CFile::begin );
			break;
		}
		ZeroMemory( szFileName, sizeof( szFileName ) );
	}
	delete [] pHeader;

	if( m_bResouceInFile == TRUE )
		return TRUE;
	else
	{
		m_File.Close();
		return FALSE;
	}
}
*/

#ifdef __BSXPACK
LPVOID CResFile::Read( void )
{
	if( !m_bResouceInFile )
		return CFileIO::Read();
	else
	{
		this->Seek((-4), CFile::current);
		int DecompressedSize = this->GetDW();

		long size = m_nFileSize;
		char* ptr = new char[ DecompressedSize ];
		if( !ptr ) 
			return NULL;

		m_File.Read( ptr, size );
		m_nFileCurrentPosition += size;

		std::vector<char> DecompressedFileData;
		filtering_istream in;
		in.push(gzip_decompressor());
		in.push(array_source(&ptr[0], size));
		iostreams::copy(in, iostreams::back_inserter(DecompressedFileData));
		copy(DecompressedFileData.begin(), DecompressedFileData.end(), ptr);
		DecompressedFileData.clear();

		return ptr;
	}
}
#else
LPVOID CResFile::Read( void )
{
	if( !m_bResouceInFile )
		return CFileIO::Read();
	else
	{
		LONG size = m_nFileSize;
		LPVOID ptr = new BYTE[ size ];
		if( !ptr ) 
			return NULL;
		//m_File.Seek( m_nFileCurrentPosition, CFile::begin );
		//m_nFileCurrentPosition 
		m_File.Read( ptr, size );
		m_nFileCurrentPosition += size;
		// 암호화일 경우 암호를 푼다.
		if( IsEncryption() )
		{
			for( int i = 0; i < size;  i++ )
				((BYTE*)ptr)[ i ] = Decryption( m_byEncryptionKey, ((BYTE*)ptr)[ i ] );
		}
		return ptr;
	}
}
#endif

#ifdef __BSXPACK
size_t CResFile::Read( void *ptr, size_t size, size_t n /* = 1  */ )
{
	if( !m_bResouceInFile )
		return CFileIO::Read( ptr, size, n );
	else
	{
		char* tmpptr = new char[size * n];
		
		size_t size_  = m_File.Read( tmpptr, size * n );
		m_nFileCurrentPosition += size_;

		std::vector<char> DecompressedFileData;
		filtering_istream in;
		in.push(gzip_decompressor());
		in.push(array_source(&tmpptr[0], size_));
		iostreams::copy(in, iostreams::back_inserter(DecompressedFileData));

		char* ctptr = (char*)ptr;
		copy(DecompressedFileData.begin(), DecompressedFileData.end(), ctptr);
		size_ = DecompressedFileData.size();

		DecompressedFileData.clear();

		ptr = (void*)ctptr;
		

		tmpptr = 0;
		safe_delete_array(tmpptr);
		ctptr = 0;
		safe_delete_array(ctptr);
		
		return size_;
	}
}

#else
size_t CResFile::Read( void *ptr, size_t size, size_t n /* = 1  */ )
{
	if( !m_bResouceInFile )
		return CFileIO::Read( ptr, size, n );
	else
	{
		//m_File.Seek( m_nFileCurrentPosition, CFile::begin );
		size_t size_  = m_File.Read( ptr, size * n );
		m_nFileCurrentPosition += size_;
		// 암호화일 경우 암호를 푼다.
		if( IsEncryption() )
		{
			for( int i = 0; i < (int)( size_ );  i++ )
				((BYTE*)ptr)[ i ] = Decryption( m_byEncryptionKey, ((BYTE*)ptr)[ i ] );
		}
#ifdef __SECURITY_0628
		CString strResFile	= m_File.GetFileName();
		CString strFile	= m_szFileName;
//		if( strResFile.Find( "data.res", 0 ) >= 0
//			|| strResFile.Find( "dataSub1.res", 0 ) >= 0
//			|| strResFile.Find( "dataSub2.res", 0 ) >= 0 )
		if( strFile.Find( "\\", 0 ) < 0 )
		{
			char sFile[100]	= { 0,};
			char sData[100]	= { 0,};
			md5( sFile, m_szFileName );
			md5( sData, (BYTE*)ptr, size );
			map<string, string>::iterator i	= CResFile::m_mapAuth.find( sFile );
			if( i != CResFile::m_mapAuth.end() )
			{
				if( lstrcmp( sData, i->second.data() ) != 0 )
				{
//					TRACE( "%s is modified\n", m_szFileName );
					::Error( "killed by CResFile::Read() %s, 3", m_szFileName );
					ExitProcess( -1 );
				}
			}
			/*
			else
			{
				TRACE( "%s:%s not found\n", m_szFileName, sFile );
				MessageBox( g_Neuz.GetSafeHwnd(), "resource not found", "error", MB_OK );
				ExitProcess( -1 );
			}
			*/
		}
#endif	// __SECURITY_0628
		return size_;
	}
}
#endif

long CResFile::GetLength( void )
{
	if( !m_bResouceInFile )
		return CFileIO::GetLength();
	else
		return m_nFileSize;
}

int CResFile::Seek( long offset, int whence )
{
	if( !m_bResouceInFile )
		return CFileIO::Seek( offset, whence );
	else
	{
		if( whence == SEEK_SET )
		{
			m_nFileCurrentPosition = m_nFileBeginPosition + offset;
			#ifdef __BSXPACK_OBJ
			this->TmpPtrPosition = offset;
			#endif
			m_File.Seek( m_nFileCurrentPosition, CFile::begin );
		}
		else 
		if( whence == SEEK_END )
		{
			m_nFileCurrentPosition = m_nFileEndPosition - offset;
			m_File.Seek( m_nFileCurrentPosition, CFile::end );//CFile::current );
		}
		else 
		if( whence == SEEK_CUR )
		{
			m_nFileCurrentPosition += offset;
			#ifdef __BSXPACK_OBJ
			this->TmpPtrPosition += offset;
			#endif
			//m_File.GetPosition() + offset
			int n = (int)( m_File.GetPosition() );
			m_File.Seek( offset, CFile::current );
		}
		else
			return -1;
		return m_nFileCurrentPosition;
	}
}

long CResFile::Tell()
{
	if( !m_bResouceInFile )
		return CFileIO::Tell();
	else
		return m_nFileCurrentPosition - m_nFileBeginPosition;
}

char CResFile::GetC( void )
{
	if( !m_bResouceInFile )
		return CFileIO::GetC();
	else
	{
		char c;
		m_File.Read( &c, 1 );
		m_nFileCurrentPosition += sizeof( c );
		// 암호화일 경우 암호를 푼다.
#ifndef __BSXPACK
		if( IsEncryption() )
			c = Decryption( m_byEncryptionKey, c );
#endif
		return c;
	}
}

WORD CResFile::GetW( void )
{
	if( !m_bResouceInFile )
		return CFileIO::GetW();
	else
	{
		WORD w;
		m_File.Read( &w, sizeof( w ) );
		m_nFileCurrentPosition += sizeof( w );
#ifndef __BSXPACK
		if( IsEncryption() )
		{
			BYTE h, l;
			h = Decryption( m_byEncryptionKey, w >> 8 );
			l = Decryption( m_byEncryptionKey, w & 0x00ff );
			return h << 8 | l;
		}
#endif
		return w;
	}
}

#pragma warning( disable : 4700 ) // CResFile::GetDW 함수내의 초기화 warning을 무시합니다.

DWORD CResFile::GetDW( void )
{
	if( !m_bResouceInFile )
		return CFileIO::GetDW();
	else
	{
		DWORD dw;
		m_File.Read( &dw, sizeof( dw ) );
		m_nFileCurrentPosition += sizeof( dw );
#ifndef __BSXPACK
		if( IsEncryption() )
		{
			BYTE a,b,c,d;
			a = Decryption( m_byEncryptionKey, ( a & 0xff000000 ) >> 24 );
			b = Decryption( m_byEncryptionKey, ( b & 0x00ff0000 ) >> 16 );
			c = Decryption( m_byEncryptionKey, ( c & 0x0000ff00 ) >> 8 );
			d = Decryption( m_byEncryptionKey, ( d & 0x000000ff ) );
			return a << 24 | b << 16 | c << 8 | d;
		}
#endif
		return dw;
	}
}
#pragma warning( default : 4700 ) // 다시 되돌립니다.


int	CResFile::Flush( void )
{
	if( !m_bResouceInFile )
		return CFileIO::Flush();
	else
	{
		m_File.Flush();
		return 0;
	}
}
#endif	// __CLIENT

CFileFinder::CFileFinder()
{
	m_pos = 0;
	m_lHandle = 0;
	m_bResFile = FALSE;
}
CFileFinder::~CFileFinder()
{
}

BOOL CFileFinder::WildCmp( LPCTSTR lpszWild, LPCTSTR lpszString ) 
{
	char* wild = (char*)lpszWild;
    char* string = (char*)lpszString;
	char* cp,* mp;

	while ((*string) && (*wild != '*')) 
	{
		if ((*wild != *string) && (*wild != '?')) 
		{
			return 0;
		}
		wild++;
		string++;
	}
	while (*string) 
	{
		if (*wild == '*') 
		{
			if (!*++wild) 
			{
				return 1;
			}
			mp = wild;
			cp = string+1;
		} 
		else 
		if ((*wild == *string) || (*wild == '?')) 
		{
			wild++;
			string++;
		} 
		else 
		{
			wild = mp;
			string = cp++;
		}
	}
	while (*wild == '*') 
	{
		wild++;
	}
	return !*wild;
}

BOOL CFileFinder::FindFirst( LPCTSTR lpFilespec, struct _finddata_t *fileinfo )
{
#if defined( __CLIENT )
	CHAR filespec[ MAX_PATH ];
	strcpy( filespec, lpFilespec );
	strlwr( filespec );

	CHAR szPath [ MAX_PATH ];
	CHAR szFile[ MAX_PATH ];

	TCHAR drive[_MAX_DRIVE], dir[_MAX_DIR], name[ _MAX_FNAME ], ext[_MAX_EXT];
	_splitpath( lpFilespec, drive, dir, name, ext );

	// 드라이브명이 들어 있으면 완전한 path다. 그러므로 그냥 카피 
	if( drive[ 0 ] )
	{
		_tcscpy( m_szFilespec, lpFilespec );
	}
	// 드리이브명이 없으면 현재 path의 연장이다. 
	else
	{
		::GetCurrentDirectory( MAX_PATH, szPath );
		_tcscpy( m_szFilespec, szPath );
		_tcscat( m_szFilespec, "\\" );
		_tcscat( m_szFilespec, lpFilespec );
	}
	strlwr( m_szFilespec );

	CString strNameName;
	RESOURCE* lpRes;
	m_pos = CResFile::m_mapResource.GetStartPosition();
	while( m_pos )
	{
		CResFile::m_mapResource.GetNextAssoc( m_pos, strNameName, (void*&) lpRes );
		_tcscpy( szFile, szPath );
		_tcscat( szFile, "\\" );
		_tcscat( szFile, strNameName );
		strlwr( szFile );

		if( WildCmp( m_szFilespec, szFile ) )
		{
			_splitpath( strNameName, drive, dir, name, ext );
			strcpy( fileinfo->name, name );
			strcat( fileinfo->name, ext );
			fileinfo->size = lpRes->dwFileSize;
			fileinfo->attrib = _A_NORMAL;
			m_bResFile = TRUE;
			return TRUE;
		}
	}
#endif	// __CLIENT
	m_bResFile = 0;
	m_lHandle = _findfirst( lpFilespec, fileinfo );
	if( m_lHandle == -1 ) 
		return FALSE;
	return TRUE;
}
BOOL CFileFinder::FindNext( struct _finddata_t *fileinfo )
{
#if defined( __CLIENT )
	if( m_bResFile == 1 )
	{
		TCHAR drive[_MAX_DRIVE], dir[_MAX_DIR], name[ _MAX_FNAME ], ext[_MAX_EXT];
		CHAR szPath [MAX_PATH];
		CHAR szFile[MAX_PATH];
		CString strNameName;
		RESOURCE* lpRes;
		::GetCurrentDirectory( MAX_PATH, szPath );

		while( m_pos )
		{
			CResFile::m_mapResource.GetNextAssoc( m_pos, strNameName, (void*&) lpRes );
			_tcscpy( szFile, szPath );
			_tcscat( szFile, "\\" );
			_tcscat( szFile, strNameName );
			strlwr( szFile );

			if( WildCmp( m_szFilespec, szFile ) )
			{
				_splitpath( strNameName, drive, dir, name, ext );
				strcpy( fileinfo->name, name );
				strcat( fileinfo->name, ext );
				fileinfo->size = lpRes->dwFileSize;
				fileinfo->attrib = _A_NORMAL;
				return TRUE;
			}
		}
		return FALSE;
	}
#endif	// __CLIENT
	if( _findnext( m_lHandle, fileinfo ) == 0 )
		return TRUE;
	return FALSE;
}
void CFileFinder::FindClose()
{
	if( m_bResFile == 0 && m_lHandle != -1 )
		_findclose( m_lHandle );
}


#ifdef __SECURITY_0628
#ifdef __CLIENT
void	CResFile::LoadAuthFile( void )
{
	CFile a;
	if( a.Open( "Flyff.a", CFile::modeRead ) == FALSE )
		return;
	int nLen	= (int)( a.GetLength() );
	BYTE* ptr	= new BYTE[nLen];
	a.Read( ptr, nLen );
	int nOffset	= 0;
	char sFile[100]	= { 0,};
	char sData[100]	= { 0,};
	while( nOffset < nLen )
	{
		memcpy( sFile, ptr + nOffset, 32 );
		sFile[32]	= '\0';
		nOffset	+= 32;
		memcpy( sData, ptr + nOffset, 32 );
		sData[32]	= '\0';
		nOffset	+= 32;
		TRACE( "%s%s\n", sFile, sData );
		bool bResult	= CResFile::m_mapAuth.insert( map<string, string>::value_type( sFile, sData ) ).second;
//		if( bResult == false )
//		{
//			MessageBox( g_Neuz.GetSafeHwnd(), prj.GetText(TID_GAME_RESOURCE_MODIFIED), "error", MB_OK );
//			ExitProcess( -1 );
//		}
	}
	md5( m_szResVer, ptr, nLen );
	safe_delete_array( ptr );
	a.Close();
}
#endif	// __CLIENT
#endif	// __SECURITY_0628