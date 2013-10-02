/* histogram cumulativisation
 *
 * Author: N. Dessipris
 * Written on: 02/08/1990
 * 24/5/95 JC
 *	- tidied up and ANSIfied
 * 20/7/95 JC
 *	- smartened up again
 *	- now works for hists >256 elements
 * 3/3/01 JC
 *	- broken into cum and norm ... helps im_histspec()
 *	- better behaviour for >8 bit hists
 * 31/10/05 JC
 * 	- was broken for vertical histograms, gah
 * 	- neater im_histnorm()
 * 23/7/07
 * 	- eek, off by 1 for more than 1 band hists
 * 12/5/08
 * 	- histcum works for signed hists now as well
 * 24/3/10
 * 	- gtkdoc
 * 	- small cleanups
 * 12/8/13	
 * 	- redone im_histcum() as a class, vips_conv()
 */

/*

    This file is part of VIPS.
    
    VIPS is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA

 */

/*

    These files are distributed with VIPS - http://www.vips.ecs.soton.ac.uk

 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#include <stdio.h>

#include <vips/vips.h>

#include "pconvolution.h"

typedef struct {
	VipsConvolution parent_instance;

	VipsPrecision precision; 
	int layers; 
	int cluster; 
} VipsConv;

typedef VipsConvolutionClass VipsConvClass;

G_DEFINE_TYPE( VipsConv, vips_conv, VIPS_TYPE_CONVOLUTION );

static int
vips_conv_build( VipsObject *object )
{
	VipsConvolution *convolution = (VipsConvolution *) object;
	VipsConv *conv = (VipsConv *) object;

	INTMASK *imsk;
	DOUBLEMASK *dmsk;

	g_object_set( conv, "out", vips_image_new(), NULL ); 

	if( VIPS_OBJECT_CLASS( vips_conv_parent_class )->build( object ) )
		return( -1 );


	switch( conv->precision ) { 
	case VIPS_PRECISION_INTEGER:
		if( !(imsk = im_vips2imask( convolution->M, "im_stats" )) || 
			!im_local_imask( convolution->out, imsk ) ||
			im_conv( convolution->in, convolution->out, imsk ) )
			return( -1 ); 
		break;

	case VIPS_PRECISION_FLOAT:
		if( !(dmsk = im_vips2mask( convolution->M, "im_stats" )) || 
			!im_local_dmask( convolution->out, dmsk ) ||
			im_conv_f( convolution->in, convolution->out, dmsk ) )
			return( -1 ); 
		break;

	case VIPS_PRECISION_APPROXIMATE:
		if( !(dmsk = im_vips2mask( convolution->M, "im_stats" )) || 
			!im_local_dmask( convolution->out, dmsk ) ||
			im_aconv( convolution->in, convolution->out, dmsk, 
				conv->layers, conv->cluster ) )
			return( -1 ); 
		break;

	default:
		g_assert( 0 );
	}

	return( 0 );
}

static void
vips_conv_class_init( VipsConvClass *class )
{
	GObjectClass *gobject_class = G_OBJECT_CLASS( class );
	VipsObjectClass *object_class = (VipsObjectClass *) class;

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "conv";
	object_class->description = _( "convolution operation" );
	object_class->build = vips_conv_build;

	VIPS_ARG_ENUM( class, "precision", 103, 
		_( "Precision" ), 
		_( "Convolve with this precision" ),
		VIPS_ARGUMENT_OPTIONAL_INPUT, 
		G_STRUCT_OFFSET( VipsConv, precision ), 
		VIPS_TYPE_PRECISION, VIPS_PRECISION_INTEGER ); 

	VIPS_ARG_INT( class, "layers", 103, 
		_( "Layers" ), 
		_( "Use this many layers in approximation" ),
		VIPS_ARGUMENT_OPTIONAL_INPUT, 
		G_STRUCT_OFFSET( VipsConv, layers ), 
		1, 1000, 5 ); 

	VIPS_ARG_INT( class, "cluster", 103, 
		_( "Cluster" ), 
		_( "Cluster lines closer than this in approximation" ),
		VIPS_ARGUMENT_OPTIONAL_INPUT, 
		G_STRUCT_OFFSET( VipsConv, cluster ), 
		1, 100, 1 ); 

}

static void
vips_conv_init( VipsConv *conv )
{
	conv->precision = VIPS_PRECISION_INTEGER;
	conv->layers = 5;
	conv->cluster = 1;
}

/**
 * vips_conv:
 * @in: input image
 * @out: output image
 * @mask: convolve with this mask
 * @...: %NULL-terminated list of optional named arguments
 *
 * Optional arguments:
 *
 * @precision: calculation accuracy
 * @layers: number of layers for approximation
 * @cluster: cluster lines closer than this distance
 *
 * Convolution. 
 *
 * Perform a convolution of @in with @mask.
 *
 * The output image 
 * always has the same #VipsBandFmt as the input image. 
 *
 * Larger values for @n_layers give more accurate
 * results, but are slower. As @n_layers approaches the mask radius, the
 * accuracy will become close to exact convolution and the speed will drop to 
 * match. For many large masks, such as Gaussian, @n_layers need be only 10% of
 * this value and accuracy will still be good.
 *
 * Smaller values of @cluster will give more accurate results, but be slower
 * and use more memory. 10% of the mask radius is a good rule of thumb.
 *
 * Returns: 0 on success, -1 on error
 */
int 
vips_conv( VipsImage *in, VipsImage **out, VipsImage *mask, ... )
{
	va_list ap;
	int result;

	va_start( ap, mask );
	result = vips_call_split( "conv", ap, in, out, mask );
	va_end( ap );

	return( result );
}
