/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2009 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#include "header.h"

Finfo::Finfo( OpFunc op, const string& name )
	: op_( op ), name_( name )
{
	;
}

unsigned int Finfo::op( Eref e, const void* buf ) const
{
	return op_( e, buf );
}

const string& Finfo::name( ) const
{
	return name_;
}

void Finfo::registerOpFuncs( 
	map< OpFunc, FuncId >& fm, 
	vector< OpFunc >& funcs )
{
	;
}
