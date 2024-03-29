/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2010 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#include "header.h"
#include "lookupVolumeFromMesh.h"
#include "PoolBase.h"
#include "Pool.h"

#define EPSILON 1e-15

const Cinfo* Pool::initCinfo()
{
		//////////////////////////////////////////////////////////////
		// Field Definitions: All inherited from PoolBase.
		//////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////
		// MsgDest Definitions: All but increment and decrement inherited
		//////////////////////////////////////////////////////////////
		static DestFinfo increment( "increment",
			"Increments mol numbers by specified amount. Can be +ve or -ve",
			new OpFunc1< Pool, double >( &Pool::increment )
		);

		static DestFinfo decrement( "decrement",
			"Decrements mol numbers by specified amount. Can be +ve or -ve",
			new OpFunc1< Pool, double >( &Pool::decrement )
		);

		//////////////////////////////////////////////////////////////
		// SrcFinfo Definitions: All inherited.
		//////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////
		// SharedMsg Definitions: All inherited.
		//////////////////////////////////////////////////////////////
	static Finfo* poolFinfos[] = {
		&increment,			// DestFinfo
		&decrement,			// DestFinfo
	};

	static Dinfo< Pool > dinfo;
	static Cinfo poolCinfo (
		"Pool",
		PoolBase::initCinfo(),
		poolFinfos,
		sizeof( poolFinfos ) / sizeof ( Finfo* ),
		&dinfo
	);

	return &poolCinfo;
}

//////////////////////////////////////////////////////////////
// Class definitions
//////////////////////////////////////////////////////////////
static const Cinfo* poolCinfo = Pool::initCinfo();
const SrcFinfo1< double >& nOut = 
	*dynamic_cast< const SrcFinfo1< double >* >( 
	poolCinfo->findFinfo( "nOut" ) );

Pool::Pool()
	: n_( 0.0 ), nInit_( 0.0 ), diffConst_( 0.0 ),
		A_( 0.0 ), B_( 0.0 ), species_( 0 )
{;}

Pool::~Pool()
{;}

//////////////////////////////////////////////////////////////
// MsgDest Definitions
//////////////////////////////////////////////////////////////

void Pool::vProcess( const Eref& e, ProcPtr p )
{
	// double A = e.sumBuf( aSlot );
	// double B = e.sumBuf( bSlot );
		/*
	if ( n_ < 0 )
		cout << "nugh" << e.objId().path() << endl;
	if ( B_ < 0 )
		cout << "bugh" << e.objId().path() << endl;
	if ( p->dt < 0 )
		cout << "tugh" << e.objId().path() << endl;
		*/

	if ( n_ > EPSILON && B_ > EPSILON ) {
		double C = exp( -B_ * p->dt / n_ );
		n_ *= C + (A_ / B_ ) * ( 1.0 - C );
	} else {
		n_ += ( A_ - B_ ) * p->dt;
		if ( n_ < 0.0 )
			n_ = 0.0;
	}

	A_ = B_ = 0.0;

	nOut.send( e, n_ );
}

void Pool::vReinit( const Eref& e, ProcPtr p )
{
	A_ = B_ = 0.0;
	n_ = getNinit( e );

	nOut.send( e, n_ );
}

void Pool::vReac( double A, double B )
{
	A_ += A;
	B_ += B;
}

void Pool::increment( double val )
{
	if ( val > 0 )
		A_ += val;
	else
		B_ -= val;
}

void Pool::decrement( double val )
{
	if ( val < 0 )
		A_ -= val;
	else
		B_ += val;
}

/*
void Pool::vRemesh( const Eref& e,
	double oldvol,
	unsigned int numTotalEntries, unsigned int startEntry, 
	const vector< unsigned int >& localIndices, 
	const vector< double >& vols )
{
	if ( e.dataIndex() != 0 )
		return;
	Neutral* n = reinterpret_cast< Neutral* >( e.data() );
	assert( vols.size() > 0 );
	double concInit = nInit_ / ( NA * oldvol );
	if ( vols.size() != e.element()->dataHandler()->localEntries() )
		n->setLastDimension( e, q, vols.size() );
	// Note that at this point the Pool pointer may be invalid!
	// But we need to update the concs anyway.
	assert( e.element()->dataHandler()->localEntries() == vols.size() );
	Pool* pooldata = reinterpret_cast< Pool* >( e.data() );
	for ( unsigned int i = 0; i < vols.size(); ++i ) {
		pooldata[i].nInit_ = pooldata[i].n_ = concInit * vols[i] * NA;
	}
}
*/

void Pool::vHandleMolWt( const Eref& e, double v )
{
	; // Here I should update DiffConst too.
}

//////////////////////////////////////////////////////////////
// Field Definitions
//////////////////////////////////////////////////////////////

void Pool::vSetN( const Eref& e, double v )
{
	n_ = v;
}

double Pool::vGetN( const Eref& e ) const
{
	return n_;
}

void Pool::vSetNinit( const Eref& e, double v )
{
	nInit_ = v;
}

double Pool::vGetNinit( const Eref& e ) const
{
	return nInit_;
}

// Conc is given in millimolar. Volume is in m^3
void Pool::vSetConc( const Eref& e, double c ) 
{
	n_ = NA * c * lookupVolumeFromMesh( e );
}

// Returns conc in millimolar.
double Pool::vGetConc( const Eref& e ) const
{
	return (n_ / NA) / lookupVolumeFromMesh( e );
}

void Pool::vSetConcInit( const Eref& e, double c )
{
	nInit_ = NA * c * lookupVolumeFromMesh( e );
}

double Pool::vGetConcInit( const Eref& e ) const
{
	return ( nInit_ / NA ) / lookupVolumeFromMesh( e );
}

void Pool::vSetDiffConst( const Eref& e, double v )
{
	diffConst_ = v;
}

double Pool::vGetDiffConst( const Eref& e ) const
{
	return diffConst_;
}

void Pool::vSetVolume( const Eref& e,  double v )
{
	assert( 0 ); // Don't currently know how to do this.
}

double Pool::vGetVolume( const Eref& e ) const
{
	return lookupVolumeFromMesh( e );
}

void Pool::vSetSpecies( const Eref& e,  SpeciesId v )
{
	species_ = v;
}

SpeciesId Pool::vGetSpecies( const Eref& e ) const
{
	return species_;
}
