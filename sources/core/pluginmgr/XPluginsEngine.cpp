/*
    Plug-ins' management library of Computer Vision Sandbox

    Copyright (C) 2011-2019, cvsandbox
    http://www.cvsandbox.com/contacts.html

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <string>
#include <queue>
#include "XPluginsEngine.hpp"

#ifdef WIN32
    #include <windows.h>
#else
    #include <sys/types.h>
    #include <dirent.h>
#endif

using namespace std;
using namespace CVSandbox;

XPluginsEngine::XPluginsEngine( ) :
    mFamilies( XFamiliesCollection::GetBuiltInFamilies( ) ),
    mModules( XModulesCollection::Create( ) )
{
}

XPluginsEngine::~XPluginsEngine( )
{
#ifdef _MSC_VER
    /*
        Some of the plug-ins use OpenMP to do parallel image processing. Unloading a DLL,
        which is built with OpenMP may cause a crash as described below:
        https://stackoverflow.com/questions/34439956/vc-crash-when-freeing-a-dll-built-with-openmp

        ------------------------------------------------------------------------------------------
        For optimal performance, the openmp threadpool spin waits for about a second prior to
        shutting down in case more work becomes available. If you unload a DLL that's in the process
        of spin-waiting, it will crash in the manner you see (most of the time).

        You can tell openmp not to spin-wait and the threads will immediately block after the loop
        finishes. Just set OMP_WAIT_POLICY=passive in your environment, or call
        SetEnvironmentVariable(L"OMP_WAIT_POLICY", L"passive"); in your function before loading
        the dll. The default is "active" which tells the threadpool to spin wait. Use the
        environment variable, or just wait a few seconds before calling FreeLibrary.
        ---

        So we choose to wait for a bit before unloading plug-in DLLs. Let the OMP threads
        spin for a bit, as by default, so that extra work could be handled without the need of
        creating threads again.
    */

    Sleep( 1200 );
#endif
}

const shared_ptr<XPluginsEngine> XPluginsEngine::Create( )
{
    return shared_ptr<XPluginsEngine>( new XPluginsEngine( ) );
}

// Collect modules containing plug-ins of the specified types
size_t XPluginsEngine::CollectModules( const string& startPath, PluginType typesToCollect )
{
    bool rootIsDone = false;

#ifdef WIN32
    const wchar_t* moduleExtension = XModuleDefaultExtensionW( );
    size_t         extLen          = wcslen( moduleExtension );
    int            charsRequired   = MultiByteToWideChar( CP_UTF8, 0, startPath.c_str( ), -1, NULL, 0 );

    if ( charsRequired > 0 )
    {
        wchar_t* wStartPath = (wchar_t*) malloc( sizeof( wchar_t ) * charsRequired );

        if ( wStartPath != nullptr )
        {
            if ( MultiByteToWideChar( CP_UTF8, 0, startPath.c_str( ), -1, wStartPath, charsRequired ) > 0 )
            {
                queue<wstring>   folders;
                folders.push( wstring( wStartPath ) );

                while ( folders.size( ) != 0 )
                {
                    const wstring path       = folders.front( );
                    wstring       searchPath = path + L"*";

                    // find all files satisfying the search string
                    HANDLE           hFind;
                    WIN32_FIND_DATAW ffData;

                    if ( ( hFind = FindFirstFileW( searchPath.c_str( ), &ffData ) ) != INVALID_HANDLE_VALUE )
                    {
                        do
                        {
                            if ( ( ( ffData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 ) &&
                                 ( !rootIsDone ) &&
                                 ( wcscmp( ffData.cFileName, L"." ) != 0 ) &&
                                 ( wcscmp( ffData.cFileName, L".." ) != 0 ) )
                            {
                                folders.push( path + ffData.cFileName + L"/" );
                            }
                            else
                            {
                                size_t fileNameLen = wcslen( ffData.cFileName );

                                if ( ( fileNameLen > extLen ) &&
                                     ( wcscmp( &ffData.cFileName[fileNameLen - extLen], moduleExtension ) == 0 ) )
                                {
                                    wstring wMoulePath    = path + ffData.cFileName;
                                    int     bytesRequired = WideCharToMultiByte( CP_UTF8, 0, wMoulePath.c_str( ), -1, NULL, 0, NULL, NULL );
                                    char*   modulePath    = (char*) malloc( bytesRequired );

                                    if ( modulePath != nullptr )
                                    {
                                        if ( WideCharToMultiByte( CP_UTF8, 0, wMoulePath.c_str( ), -1, modulePath, bytesRequired, NULL, NULL ) > 0 )
                                        {
                                            LoadMoadule( string( modulePath ), typesToCollect );
                                        }

                                        free( modulePath );
                                    }
                                }
                            }
                        }
                        while ( FindNextFileW( hFind, &ffData ) );

                        FindClose( hFind );
                    }

                    folders.pop( );
                    rootIsDone = true;
                }
            }

            free( wStartPath );
        }
    }
#else
    const char*     moduleExtension = XModuleDefaultExtension( );
    size_t          extLen = strlen( moduleExtension );
    queue<string>   folders;

    folders.push( startPath );

    while ( folders.size( ) != 0 )
    {
        const string path = folders.front( );

        // search for files
        DIR* dirHandle;
        struct dirent* dirEntry;

        if ( ( dirHandle = opendir( path.c_str( ) ) ) != 0 )
        {
            while ( ( dirEntry = readdir( dirHandle ) ) != 0 )
            {
                size_t fileNameLen = strlen( dirEntry->d_name );

                if ( ( fileNameLen > extLen ) &&
                     ( strcmp( &dirEntry->d_name[fileNameLen - extLen], moduleExtension ) == 0 ) )
                {
                    LoadMoadule( path + dirEntry->d_name, typesToCollect );
                }
            }

            closedir( dirHandle );
        }

        folders.pop( );
        rootIsDone = true;
    }
#endif

    return mModules->Count( );
}

// Load module with the specified file name
void XPluginsEngine::LoadMoadule( const string& fileName, PluginType typesToCollect )
{
    shared_ptr<XPluginsModule> module = XPluginsModule::Create( fileName );

    if ( ( module->Load( typesToCollect ) == SuccessCode ) && ( module->Count( ) != 0 ) )
    {
        mModules->Add( module );
    }
}

// Get all plug-ins of the specified types
const shared_ptr<const XPluginsCollection> XPluginsEngine::GetPluginsOfType( PluginType typesMask ) const
{
    return mModules->GetPluginsOfType( typesMask );
}

// Get plug-in descriptor by its short name
const shared_ptr<const XPluginDescriptor> XPluginsEngine::GetPlugin( const string& shortName ) const
{
    return mModules->GetPlugin( shortName );
}

// Get plug-in descriptor by its ID
const shared_ptr<const XPluginDescriptor> XPluginsEngine::GetPlugin( const XGuid& id ) const
{
    return mModules->GetPlugin( id );
}

// Get plug-in descriptor by its ID. Also provides module descriptor for it.
const shared_ptr<const XPluginDescriptor> XPluginsEngine::GetPlugin( const XGuid& id, shared_ptr<const XPluginsModule>& moduleDesc ) const
{
    return mModules->GetPlugin( id, moduleDesc );
}

// Get module descriptor by its short name
const shared_ptr<const XPluginsModule> XPluginsEngine::GetModule( const string& shortName ) const
{
    return mModules->GetModule( shortName );
}

// Get module descriptor by its ID
const shared_ptr<const XPluginsModule> XPluginsEngine::GetModule( const XGuid& id ) const
{
    return mModules->GetModule( id );
}
