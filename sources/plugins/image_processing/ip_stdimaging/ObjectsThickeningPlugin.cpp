/*
    Standard image processing plug-ins of Computer Vision Sandbox

    Copyright (C) 2011-2018, cvsandbox
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

#include <ximaging.h>
#include "ObjectsThickeningPlugin.hpp"

// Supported pixel formats of input/output images
const XPixelFormat ObjectsThickeningPlugin::supportedFormats[] =
{
    XPixelFormatGrayscale8
};

ObjectsThickeningPlugin::ObjectsThickeningPlugin( ) :
    growingAmount( 3 ), tempDistanceMap( nullptr )
{
}

ObjectsThickeningPlugin::~ObjectsThickeningPlugin( )
{
    XImageFree( &tempDistanceMap );
}

void ObjectsThickeningPlugin::Dispose( )
{
    delete this;
}

// The plug-in can process image in-place without creating new image as a result
bool ObjectsThickeningPlugin::CanProcessInPlace( )
{
    return true;
}

// Provide supported pixel formats
XErrorCode ObjectsThickeningPlugin::GetPixelFormatTranslations( XPixelFormat* inputFormats, XPixelFormat* outputFormats, int32_t* count )
{
    return GetPixelFormatTranslationsImpl( inputFormats, outputFormats, count, supportedFormats, supportedFormats,
        sizeof( supportedFormats ) / sizeof( XPixelFormat ) );
}

// Process the specified source image and return new as a result
XErrorCode ObjectsThickeningPlugin::ProcessImage( const ximage* src, ximage** dst )
{
    XErrorCode ret = XImageClone( src, dst );

    if ( ret == SuccessCode )
    {
        ret = ProcessImageInPlace( *dst );

        if ( ret != SuccessCode )
        {
            XImageFree( dst );
        }
    }

    return ret;
}

// Process the specified source image by changing it
XErrorCode ObjectsThickeningPlugin::ProcessImageInPlace( ximage* src )
{
    XErrorCode ret = SuccessCode;

    if ( src == nullptr )
    {
        ret = ErrorNullParameter;
    }
    else
    {
        ret = XImageAllocateRaw( src->width, src->height, XPixelFormatGrayscale16, &tempDistanceMap );

        if ( ret == SuccessCode )
        {
            ret = ObjectsThickening( src, tempDistanceMap, growingAmount );
        }
    }

    return ret;
}

// Get specified property value of the plug-in
XErrorCode ObjectsThickeningPlugin::GetProperty( int32_t id, xvariant* value ) const
{
    XErrorCode ret = SuccessCode;

    switch ( id )
    {
    case 0:
        value->type = XVT_U2;
        value->value.usVal = growingAmount;
        break;

    default:
        ret = ErrorInvalidProperty;
        break;
    }

    return ret;
}

// Set specified property value of the plug-in
XErrorCode ObjectsThickeningPlugin::SetProperty( int32_t id, const xvariant* value )
{
    XErrorCode ret = SuccessCode;

    xvariant convertedValue;
    XVariantInit( &convertedValue );

    // make sure property value has expected type
    ret = PropertyChangeTypeHelper( id, value, propertiesDescription, 1, &convertedValue );

    if ( ret == SuccessCode )
    {
        switch ( id )
        {
        case 0:
            growingAmount = convertedValue.value.usVal;
            break;
        }
    }

    XVariantClear( &convertedValue );

    return ret;
}
